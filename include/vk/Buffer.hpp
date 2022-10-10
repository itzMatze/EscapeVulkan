#pragma once

#include <utility>
#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class Buffer
    {
    public:
        template<class T>
        Buffer(const VulkanMainContext& vmc, const std::vector<T>& data, vk::BufferUsageFlags usage_flags, const std::vector<uint32_t>& queue_family_indices) : vmc(vmc), device_local(false), element_count(data.size()), byte_size(sizeof(T) * data.size())
        {
            std::tie(buffer, memory) = create_buffer(usage_flags, {vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent}, queue_family_indices);
            update_data(data);
        }

        template<class T>
        Buffer(const VulkanMainContext& vmc, const std::vector<T>& data, vk::BufferUsageFlags usage_flags, std::vector<uint32_t> queue_family_indices, const vk::CommandBuffer& command_buffer) : vmc(vmc), device_local(true), element_count(data.size()), byte_size(sizeof(T) * data.size())
        {
            std::tie(buffer, memory) = create_buffer((usage_flags | vk::BufferUsageFlagBits::eTransferDst), {vk::MemoryPropertyFlagBits::eDeviceLocal}, queue_family_indices);
            update_data(data, command_buffer);
        }

        void self_destruct()
        {
            vmc.logical_device.get().destroyBuffer(buffer);
            vmc.logical_device.get().freeMemory(memory);
        }

        const vk::Buffer& get() const
        {
            return buffer;
        }

        uint64_t get_element_count() const
        {
            return element_count;
        }

        uint64_t get_byte_size() const
        {
            return byte_size;
        }

        template<class T>
        void update_data(const T& data)
        {
            update_data(std::vector<T>{data});
        }

        template<class T>
        void update_data(const std::vector<T>& data)
        {
            VE_ASSERT(sizeof(T) * data.size() <= byte_size, "Data is larger than buffer!\n");
            VE_ASSERT(!device_local, "Trying to update data to a buffer that is device local but it should not!\n");

            void* mapped_mem = vmc.logical_device.get().mapMemory(memory, 0, VK_WHOLE_SIZE);
            memcpy(mapped_mem, data.data(), sizeof(T) * data.size());
            vmc.logical_device.get().unmapMemory(memory);
        }

        template<class T>
        void update_data(const T& data, const vk::CommandBuffer& command_buffer)
        {
            update_data(std::vector<T>{data}, command_buffer);
        }

        template<class T>
        void update_data(const std::vector<T>& data, const vk::CommandBuffer& command_buffer)
        {
            VE_ASSERT(sizeof(T) * data.size() <= byte_size, "Data is larger than buffer!\n");
            VE_ASSERT(device_local, "Trying to update data to a buffer that is not device local but it should!\n");

            auto [staging_buffer, staging_memory] = create_buffer((vk::BufferUsageFlagBits::eTransferSrc), {vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent}, {uint32_t(vmc.queues_family_indices.transfer)});

            void* mapped_mem = vmc.logical_device.get().mapMemory(staging_memory, 0, VK_WHOLE_SIZE);
            memcpy(mapped_mem, data.data(), sizeof(T) * data.size());
            vmc.logical_device.get().unmapMemory(staging_memory);

            vk::CommandBufferBeginInfo cbbi{};
            cbbi.sType = vk::StructureType::eCommandBufferBeginInfo;
            cbbi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            command_buffer.begin(cbbi);
            vk::BufferCopy copy_region{};
            copy_region.srcOffset = 0;
            copy_region.dstOffset = 0;
            copy_region.size = sizeof(T) * data.size();
            command_buffer.copyBuffer(staging_buffer, buffer, copy_region);
            command_buffer.end();
            vk::SubmitInfo submit_info{};
            submit_info.sType = vk::StructureType::eSubmitInfo;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;
            vmc.get_transfer_queue().submit(submit_info);
            vmc.get_transfer_queue().waitIdle();
            command_buffer.reset();

            vmc.logical_device.get().destroyBuffer(staging_buffer);
            vmc.logical_device.get().freeMemory(staging_memory);
        }

    private:
        std::pair<vk::Buffer, vk::DeviceMemory> create_buffer(vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags mpf, const std::vector<uint32_t>& queue_family_indices)
        {
            vk::BufferCreateInfo bci{};
            bci.sType = vk::StructureType::eBufferCreateInfo;
            bci.size = byte_size;
            bci.usage = usage_flags;
            bci.sharingMode = queue_family_indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
            bci.flags = {};
            bci.queueFamilyIndexCount = queue_family_indices.size();
            bci.pQueueFamilyIndices = queue_family_indices.data();
            vk::Buffer local_buffer = vmc.logical_device.get().createBuffer(bci);
            vk::MemoryRequirements mem_requirements = vmc.logical_device.get().getBufferMemoryRequirements(local_buffer);
            vk::MemoryAllocateInfo mai{};
            mai.sType = vk::StructureType::eMemoryAllocateInfo;
            mai.allocationSize = mem_requirements.size;
            mai.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, mpf);
            vk::DeviceMemory local_memory = vmc.logical_device.get().allocateMemory(mai);
            vmc.logical_device.get().bindBufferMemory(local_buffer, local_memory, 0);
            return std::make_pair(local_buffer, local_memory);
        }

        uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties)
        {
            vk::PhysicalDeviceMemoryProperties mem_properties = vmc.physical_device.get().getMemoryProperties();
            for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i)
            {
                if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) return i;
            }
            VE_THROW("Failed to find suitable memory type!");
        }

        const VulkanMainContext& vmc;
        bool device_local;
        uint64_t byte_size;
        uint64_t element_count;
        vk::Buffer buffer;
        vk::DeviceMemory memory;
    };
}// namespace ve