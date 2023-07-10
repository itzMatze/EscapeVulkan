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

            for (auto& blas : bottomLevelAS[i])
            {
                vmc.logical_device.get().destroyAccelerationStructureKHR(blas.handle);
                storage.destroy_buffer(blas.buffer);
                storage.destroy_buffer(blas.scratch_buffer);
            }
            bottomLevelAS[i].clear();
        }
    }

    void PathTracer::create_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, const std::vector<uint32_t>& index_offsets, const std::vector<uint32_t>& index_counts, vk::DeviceSize vertex_stride, BottomLevelAccelerationStructure& blas)
    {
        Buffer& vertex_buffer = storage.get_buffer(vertex_buffer_id);
        Buffer& index_buffer = storage.get_buffer(index_buffer_id);

        vk::DeviceOrHostAddressConstKHR vertex_buffer_device_adress(vertex_buffer.get_device_address());
        vk::DeviceOrHostAddressConstKHR index_buffer_device_adress(index_buffer.get_device_address());

        std::vector<uint32_t> num_triangles;
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR> asbris;
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> pasbris;
        std::vector<vk::AccelerationStructureGeometryKHR> asgs;
        for (uint32_t i = 0; i < index_offsets.size(); ++i)
        {
            vk::AccelerationStructureBuildRangeInfoKHR asbri{};
            asbri.primitiveCount = index_counts[i] / 3;
            asbri.primitiveOffset = sizeof(uint32_t) * index_offsets[i];
            asbri.firstVertex = 0;
            asbri.transformOffset = 0;
            asbris.push_back(asbri);
            num_triangles.push_back(asbri.primitiveCount);

            vk::AccelerationStructureGeometryKHR asg{};
            asg.flags = vk::GeometryFlagBitsKHR::eOpaque;
            asg.geometryType = vk::GeometryTypeKHR::eTriangles;
            asg.geometry.triangles.sType = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
            asg.geometry.triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
            asg.geometry.triangles.vertexData = vertex_buffer_device_adress;
            asg.geometry.triangles.maxVertex = vertex_buffer.get_element_count();
            asg.geometry.triangles.vertexStride = vertex_stride;
            asg.geometry.triangles.indexType = vk::IndexType::eUint32;
            asg.geometry.triangles.indexData = index_buffer_device_adress;
            asg.geometry.triangles.transformData.deviceAddress = 0;
            asg.geometry.triangles.transformData.hostAddress = nullptr;
            asgs.push_back(asg);
        }
        pasbris.push_back(asbris.data());

        vk::AccelerationStructureBuildGeometryInfoKHR asbgi{};
        asbgi.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
        asbgi.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        asbgi.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
        asbgi.geometryCount = asgs.size();
        asbgi.pGeometries = asgs.data();

        if (!blas.is_built)
        {
            vk::AccelerationStructureBuildSizesInfoKHR asbsi = vmc.logical_device.get().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, asbgi, num_triangles);

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
        }

        asbgi.dstAccelerationStructure = blas.handle;
        asbgi.scratchData.deviceAddress = storage.get_buffer(blas.scratch_buffer).get_device_address();
        std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> asbgis{};
        asbgis.push_back(asbgi);

        cb.buildAccelerationStructuresKHR(asbgis, pasbris);
        vk::BufferMemoryBarrier buffer_memory_barrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR, vk::AccessFlagBits::eAccelerationStructureReadKHR, vmc.queue_family_indices.compute, vmc.queue_family_indices.compute, storage.get_buffer(blas.buffer).get(), 0, storage.get_buffer(blas.buffer).get_byte_size());
        cb.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, vk::DependencyFlagBits::eDeviceGroup, {}, {buffer_memory_barrier}, {});
        blas.is_built = true;
    }

    uint32_t PathTracer::add_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, const std::vector<uint32_t>& index_offsets, const std::vector<uint32_t>& index_counts, vk::DeviceSize vertex_stride) 
    {
        bottomLevelAS[0].push_back(BottomLevelAccelerationStructure{});
        create_blas(cb, vertex_buffer_id, index_buffer_id, index_offsets, index_counts, vertex_stride, bottomLevelAS[0].back());
        bottomLevelAS[1].push_back(BottomLevelAccelerationStructure{});
        create_blas(cb, vertex_buffer_id, index_buffer_id, index_offsets, index_counts, vertex_stride, bottomLevelAS[1].back());
        return bottomLevelAS[0].size() - 1;
    }

    void PathTracer::update_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, const std::vector<uint32_t>& index_offsets, const std::vector<uint32_t>& index_counts, uint32_t blas_idx, uint32_t frame_idx, vk::DeviceSize vertex_stride)
    {
        create_blas(cb, vertex_buffer_id, index_buffer_id, index_offsets, index_counts, vertex_stride, bottomLevelAS[frame_idx][blas_idx]);
    }

    uint32_t PathTracer::add_instance(uint32_t blas_idx, const glm::mat4& M, uint32_t custom_index)
    {
        vk::AccelerationStructureInstanceKHR instance;
        instance.transform = std::array<std::array<float, 4>, 3>({std::array<float, 4>({M[0][0], M[0][1], M[0][2], M[0][3]}), std::array<float, 4>({M[1][0], M[1][1], M[1][2], M[1][3]}), std::array<float, 4>({M[2][0], M[2][1], M[2][2], M[2][3]})});
        instance.accelerationStructureReference = bottomLevelAS[0][blas_idx].deviceAddress;
        instance.instanceCustomIndex = custom_index;
        instance.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
        instance.mask = 0xFF;
        instances[0].push_back(instance);
        instance.accelerationStructureReference = bottomLevelAS[1][blas_idx].deviceAddress;
        instances[1].push_back(instance);
        return instances[0].size() - 1;
    }

    void PathTracer::update_instance(uint32_t instance_idx, const glm::mat4& M)
    {
        instances[0][instance_idx].transform = std::array<std::array<float, 4>, 3>({std::array<float, 4>({M[0][0], M[1][0], M[2][0], M[3][0]}), std::array<float, 4>({M[0][1], M[1][1], M[2][1], M[3][1]}), std::array<float, 4>({M[0][2], M[1][2], M[2][2], M[3][2]})});
        instances[1][instance_idx].transform = std::array<std::array<float, 4>, 3>({std::array<float, 4>({M[0][0], M[1][0], M[2][0], M[3][0]}), std::array<float, 4>({M[0][1], M[1][1], M[2][1], M[3][1]}), std::array<float, 4>({M[0][2], M[1][2], M[2][2], M[3][2]})});
    }

    void PathTracer::create_tlas(vk::CommandBuffer& cb, uint32_t frame_idx)
    {
        if (!topLevelAS[frame_idx].is_built)
        {
            instances_buffer[frame_idx] = storage.add_buffer(instances[frame_idx].data(), instances[frame_idx].size(), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, false, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute);
        }
        storage.get_buffer(instances_buffer[frame_idx]).update_data(instances[frame_idx]);

        vk::DeviceOrHostAddressConstKHR instance_data_device_address;
        instance_data_device_address.deviceAddress = storage.get_buffer(instances_buffer[frame_idx]).get_device_address();

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

        uint32_t primitive_count = instances[frame_idx].size();

        if (!topLevelAS[frame_idx].is_built)
        {
            vk::AccelerationStructureBuildSizesInfoKHR asbsi{};
            vmc.logical_device.get().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, &asbgi, &primitive_count, &asbsi);

            topLevelAS[frame_idx].buffer = storage.add_named_buffer("tlas_" + std::to_string(frame_idx), asbsi.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR, true, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute);

            vk::AccelerationStructureCreateInfoKHR asci{};
            asci.sType = vk::StructureType::eAccelerationStructureCreateInfoKHR;
            asci.buffer = storage.get_buffer(topLevelAS[frame_idx].buffer).get();
            asci.size = asbsi.accelerationStructureSize;
            asci.type = vk::AccelerationStructureTypeKHR::eTopLevel;
            topLevelAS[frame_idx].handle = vmc.logical_device.get().createAccelerationStructureKHR(asci);

            vk::AccelerationStructureDeviceAddressInfoKHR asdai{};
            asdai.sType = vk::StructureType::eAccelerationStructureDeviceAddressInfoKHR;
            asdai.accelerationStructure = topLevelAS[frame_idx].handle;
            topLevelAS[frame_idx].deviceAddress = vmc.logical_device.get().getAccelerationStructureAddressKHR(&asdai);

            wdsas[frame_idx].accelerationStructureCount = 1;
            wdsas[frame_idx].pAccelerationStructures = &(topLevelAS[frame_idx].handle);
            storage.get_buffer(topLevelAS[frame_idx].buffer).pNext = &(wdsas[frame_idx]);

            topLevelAS[frame_idx].scratch_buffer = storage.add_buffer(asbsi.buildScratchSize, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, true, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute); 
        }

        asbgi.dstAccelerationStructure = topLevelAS[frame_idx].handle;
        asbgi.scratchData.deviceAddress = storage.get_buffer(topLevelAS[frame_idx].scratch_buffer).get_device_address();

        vk::AccelerationStructureBuildRangeInfoKHR asbri{};
        asbri.primitiveCount = instances[frame_idx].size();
        asbri.primitiveOffset = 0;
        asbri.firstVertex = 0;
        asbri.transformOffset = 0;
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> asbris = {&asbri};

        cb.buildAccelerationStructuresKHR(asbgi, asbris);
        topLevelAS[frame_idx].is_built = true;
    }
} // namespace ve
