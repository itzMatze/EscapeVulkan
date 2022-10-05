#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"

namespace ve
{
    class Buffer
    {
    public:
        Buffer(const vk::PhysicalDevice physical_device, const vk::Device logical_device) : device(logical_device), physical_device(physical_device)
        {
        }

        ~Buffer()
        {
            for (auto& buffer: buffers)
            {
                device.destroyBuffer(buffer);
            }
            for (auto& memory: memories)
            {
                device.freeMemory(memory);
            }
        }

        template<class T>
        void add_buffer(const std::vector<T>& data, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags mem_property_flags, const std::vector<uint32_t>& queue_family_indices)
        {
            vk::BufferCreateInfo bci{};
            bci.sType = vk::StructureType::eBufferCreateInfo;
            bci.size = sizeof(T) * data.size();
            bci.usage = usage_flags;
            bci.sharingMode = vk::SharingMode::eConcurrent;
            bci.flags = {};
            bci.queueFamilyIndexCount = 2;
            bci.pQueueFamilyIndices = queue_family_indices.data();
            buffers.push_back(device.createBuffer(bci));
            vk::MemoryRequirements mem_requirements = device.getBufferMemoryRequirements(buffers[buffers.size() - 1]);
            vk::MemoryAllocateInfo mai{};
            mai.sType = vk::StructureType::eMemoryAllocateInfo;
            mai.allocationSize = mem_requirements.size;
            mai.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, mem_property_flags);
            memories.push_back(device.allocateMemory(mai));
            device.bindBufferMemory(buffers[buffers.size() - 1], memories[memories.size() - 1], 0);
            vertices += data.size();
        }

        template<class T>
        void send_data(const std::vector<T>& data, uint32_t idx)
        {
            void* mapped_mem = device.mapMemory(memories[idx], 0, VK_WHOLE_SIZE);
            memcpy(mapped_mem, data.data(), sizeof(T) * data.size());
            device.unmapMemory(memories[idx]);
        }

        void copy_data(uint32_t idx_src, uint32_t idx_dst, vk::DeviceSize size, const vk::CommandBuffer command_buffer, const vk::Queue queue)
        {
            vk::CommandBufferBeginInfo cbbi{};
            cbbi.sType = vk::StructureType::eCommandBufferBeginInfo;
            cbbi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            command_buffer.begin(cbbi);
            vk::BufferCopy copy_region{};
            copy_region.srcOffset = 0;
            copy_region.dstOffset = 0;
            copy_region.size = size;
            command_buffer.copyBuffer(buffers[idx_src], buffers[idx_dst], copy_region);
            command_buffer.end();
            vk::SubmitInfo submit_info{};
            submit_info.sType = vk::StructureType::eSubmitInfo;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;
            queue.submit(submit_info);
            queue.waitIdle();
        }

        const std::vector<vk::Buffer>& get_buffers() const
        {
            return buffers;
        }
        uint32_t vertices = 0;

    private:
        uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties)
        {
            vk::PhysicalDeviceMemoryProperties mem_properties = physical_device.getMemoryProperties();
            for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i)
            {
                if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) return i;
            }
            VE_THROW("Failed to find suitable memory type!");
        }

        const vk::Device device;
        const vk::PhysicalDevice physical_device;
        std::vector<vk::Buffer> buffers;
        std::vector<vk::DeviceMemory> memories;
    };
}// namespace ve