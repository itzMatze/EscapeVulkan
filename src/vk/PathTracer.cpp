#include "vk/PathTracer.hpp"

namespace ve 
{
    PathTracer::PathTracer(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : vmc(vmc), vcc(vcc), storage(storage) {}

    void PathTracer::self_destruct()
    {
        for (uint32_t i = 0; i < 2; ++i)
        {
            vmc.logical_device.get().destroyAccelerationStructureKHR(topLevelAS[i].handle);
            storage.destroy_buffer(topLevelAS[i].buffer);
            storage.destroy_buffer(topLevelAS[i].scratch_buffer);
            storage.destroy_buffer(instances_buffer[i]);
        }
        for (auto& blas : bottomLevelAS)
        {
            vmc.logical_device.get().destroyAccelerationStructureKHR(blas.handle);
            storage.destroy_buffer(blas.buffer);
            storage.destroy_buffer(blas.scratch_buffer);
        }
        bottomLevelAS.clear();
    }

    BottomLevelAccelerationStructure PathTracer::create_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, uint32_t index_offset, uint32_t num_indices)
    {
        Buffer& vertex_buffer = storage.get_buffer(vertex_buffer_id);
        Buffer& index_buffer = storage.get_buffer(index_buffer_id);

        vk::DeviceOrHostAddressConstKHR vertex_buffer_device_adress(vertex_buffer.get_device_address());
        vk::DeviceOrHostAddressConstKHR index_buffer_device_adress(index_buffer.get_device_address());

        uint32_t numTriangles = num_indices / 3;

        vk::AccelerationStructureGeometryKHR asg{};
        asg.flags = vk::GeometryFlagBitsKHR::eOpaque;
        asg.geometryType = vk::GeometryTypeKHR::eTriangles;
        asg.geometry.triangles.sType = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
        asg.geometry.triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
        asg.geometry.triangles.vertexData = vertex_buffer_device_adress;
        asg.geometry.triangles.maxVertex = vertex_buffer.get_element_count();
        asg.geometry.triangles.vertexStride = sizeof(Vertex);
        asg.geometry.triangles.indexType = vk::IndexType::eUint32;
        asg.geometry.triangles.indexData = index_buffer_device_adress;
        asg.geometry.triangles.transformData.deviceAddress = 0;
        asg.geometry.triangles.transformData.hostAddress = nullptr;

        vk::AccelerationStructureBuildGeometryInfoKHR asbgi{};
        asbgi.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
        asbgi.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        asbgi.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
        asbgi.geometryCount = 1;
        asbgi.pGeometries = &asg;

        vk::AccelerationStructureBuildSizesInfoKHR asbsi{};

        vmc.logical_device.get().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &asbgi, &numTriangles, &asbsi);

        BottomLevelAccelerationStructure blas;

        blas.buffer = storage.add_buffer(asbsi.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR, true, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute);

        vk::AccelerationStructureCreateInfoKHR asci{};
        asci.sType = vk::StructureType::eAccelerationStructureCreateInfoKHR;
        asci.buffer = storage.get_buffer(blas.buffer).get();
        asci.size = asbsi.accelerationStructureSize;
        asci.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
        blas.handle = vmc.logical_device.get().createAccelerationStructureKHR(asci);

        vk::AccelerationStructureDeviceAddressInfoKHR asdai{};
        asdai.sType = vk::StructureType::eAccelerationStructureDeviceAddressInfoKHR;
        asdai.accelerationStructure = blas.handle;

        blas.deviceAddress = vmc.logical_device.get().getAccelerationStructureAddressKHR(&asdai);

        blas.scratch_buffer = storage.add_buffer(asbsi.buildScratchSize, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, true, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute); 

        asbgi.dstAccelerationStructure = blas.handle;
        asbgi.scratchData.deviceAddress = storage.get_buffer(blas.scratch_buffer).get_device_address();
        std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> asbgis{};
        asbgis.push_back(asbgi);

        vk::AccelerationStructureBuildRangeInfoKHR asbri{};
        asbri.primitiveCount = numTriangles;
        asbri.primitiveOffset = sizeof(uint32_t) * index_offset;
        asbri.firstVertex = 0;
        asbri.transformOffset = 0;
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> asbris = { &asbri };

        cb.buildAccelerationStructuresKHR(asbgis, asbris);
        vk::BufferMemoryBarrier buffer_memory_barrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR, vk::AccessFlagBits::eAccelerationStructureReadKHR, vmc.queue_family_indices.graphics, vmc.queue_family_indices.graphics, storage.get_buffer(blas.buffer).get(), 0, storage.get_buffer(blas.buffer).get_byte_size());
        cb.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, vk::DependencyFlagBits::eDeviceGroup, {}, {buffer_memory_barrier}, {});

        return blas;
    }

    uint32_t PathTracer::add_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, uint32_t index_offset, uint32_t num_indices) 
    {
        bottomLevelAS.push_back(create_blas(cb, vertex_buffer_id, index_buffer_id, index_offset, num_indices));
        return bottomLevelAS.size() - 1;
    }

    void PathTracer::update_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, uint32_t index_offset, uint32_t num_indices, uint32_t blas_idx)
    {
        vk::DeviceAddress old_device_address = bottomLevelAS[blas_idx].deviceAddress;
        storage.destroy_buffer(bottomLevelAS[blas_idx].buffer);
        bottomLevelAS[blas_idx] = create_blas(cb, vertex_buffer_id, index_buffer_id, index_offset, num_indices);
        for (vk::AccelerationStructureInstanceKHR& instance : instances)
        {
            if (instance.accelerationStructureReference == old_device_address) instance.accelerationStructureReference = bottomLevelAS[blas_idx].deviceAddress;
        }
    }

    uint32_t PathTracer::add_instance(uint32_t blas_idx, const glm::mat4& M, uint32_t custom_index)
    {
        vk::AccelerationStructureInstanceKHR instance;
        instance.transform = std::array<std::array<float, 4>, 3>({std::array<float, 4>({M[0][0], M[0][1], M[0][2], M[0][3]}), std::array<float, 4>({M[1][0], M[1][1], M[1][2], M[1][3]}), std::array<float, 4>({M[2][0], M[2][1], M[2][2], M[2][3]})});
        instance.accelerationStructureReference = bottomLevelAS[blas_idx].deviceAddress;
        instance.instanceCustomIndex = custom_index;
        instance.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
        instance.mask = 0xFF;
        instances.push_back(instance);
        return instances.size() - 1;
    }

    void PathTracer::update_instance(uint32_t instance_idx, const glm::mat4& M)
    {
        instances[instance_idx].transform = std::array<std::array<float, 4>, 3>({std::array<float, 4>({M[0][0], M[1][0], M[2][0], M[3][0]}), std::array<float, 4>({M[0][1], M[1][1], M[2][1], M[3][1]}), std::array<float, 4>({M[0][2], M[1][2], M[2][2], M[3][2]})});
    }

    void PathTracer::create_tlas(vk::CommandBuffer& cb, uint32_t idx)
    {
        if (!topLevelAS[idx].is_built)
        {
            instances_buffer[idx] = storage.add_buffer(instances.data(), instances.size(), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, false, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute);
        }
        storage.get_buffer(instances_buffer[idx]).update_data(instances);

        vk::DeviceOrHostAddressConstKHR instance_data_device_address;
        instance_data_device_address.deviceAddress = storage.get_buffer(instances_buffer[idx]).get_device_address();

        vk::AccelerationStructureGeometryKHR asg;
        asg.geometryType = vk::GeometryTypeKHR::eInstances;
        asg.flags = vk::GeometryFlagBitsKHR::eOpaque;
        asg.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
        asg.geometry.instances.arrayOfPointers = VK_FALSE;
        asg.geometry.instances.data = instance_data_device_address;

        vk::AccelerationStructureBuildGeometryInfoKHR asbgi;
        asbgi.type = vk::AccelerationStructureTypeKHR::eTopLevel;
        asbgi.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        asbgi.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
        asbgi.geometryCount = 1;
        asbgi.pGeometries = &asg;

        uint32_t primitive_count = instances.size();

        if (!topLevelAS[idx].is_built)
        {
            vk::AccelerationStructureBuildSizesInfoKHR asbsi{};
            vmc.logical_device.get().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &asbgi, &primitive_count, &asbsi);

            topLevelAS[idx].buffer = storage.add_named_buffer("tlas_" + std::to_string(idx), asbsi.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR, true, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute);

            vk::AccelerationStructureCreateInfoKHR asci{};
            asci.sType = vk::StructureType::eAccelerationStructureCreateInfoKHR;
            asci.buffer = storage.get_buffer(topLevelAS[idx].buffer).get();
            asci.size = asbsi.accelerationStructureSize;
            asci.type = vk::AccelerationStructureTypeKHR::eTopLevel;
            topLevelAS[idx].handle = vmc.logical_device.get().createAccelerationStructureKHR(asci);

            vk::AccelerationStructureDeviceAddressInfoKHR asdai{};
            asdai.sType = vk::StructureType::eAccelerationStructureDeviceAddressInfoKHR;
            asdai.accelerationStructure = topLevelAS[idx].handle;
            topLevelAS[idx].deviceAddress = vmc.logical_device.get().getAccelerationStructureAddressKHR(&asdai);

            wdsas[idx].accelerationStructureCount = 1;
            wdsas[idx].pAccelerationStructures = &(topLevelAS[idx].handle);
            storage.get_buffer(topLevelAS[idx].buffer).pNext = &(wdsas[idx]);

            topLevelAS[idx].scratch_buffer = storage.add_buffer(asbsi.buildScratchSize, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, true, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute); 
        }

        asbgi.dstAccelerationStructure = topLevelAS[idx].handle;
        asbgi.scratchData.deviceAddress = storage.get_buffer(topLevelAS[idx].scratch_buffer).get_device_address();

        vk::AccelerationStructureBuildRangeInfoKHR asbri{};
        asbri.primitiveCount = instances.size();
        asbri.primitiveOffset = 0;
        asbri.firstVertex = 0;
        asbri.transformOffset = 0;
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> asbris = {&asbri};

        cb.buildAccelerationStructuresKHR(asbgi, asbris);
        topLevelAS[idx].is_built = true;
    }
} // namespace ve
