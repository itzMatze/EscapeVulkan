#pragma once

#include <utility>
#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class Buffer
    {
    public:
        Buffer() = default;

        template<class T, class... Args>
        Buffer(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const T* data, std::size_t elements, vk::BufferUsageFlags usage_flags, bool device_local, Args... queue_family_indices) : vmc(vmc), vcc(vcc), device_local(device_local), element_count(elements), byte_size(sizeof(T) * elements)
        {
            std::vector<uint32_t> queue_family_indices_vec = {queue_family_indices...};
            if (device_local)
            {
                std::tie(buffer, vmaa) = create_buffer((usage_flags | vk::BufferUsageFlagBits::eTransferDst), {}, queue_family_indices_vec);
                update_data(data, elements);
            }
            else
            {
                std::tie(buffer, vmaa) = create_buffer(usage_flags, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, queue_family_indices_vec);
                update_data(data, elements);
            }
        }


        template<class T, class... Args>
        Buffer(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<T>& data, vk::BufferUsageFlags usage_flags, bool device_local, Args... queue_family_indices) : Buffer(vmc, vcc, data.data(), data.size(), usage_flags, device_local, queue_family_indices...)
        {}

        void self_destruct()
        {
            vmaDestroyBuffer(vmc.va, buffer, vmaa);
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
        void update_data(const T* data, std::size_t elements)
        {
            VE_ASSERT(sizeof(T) * elements <= byte_size, "Data is larger than buffer!");

            if (device_local)
            {
                auto [staging_buffer, staging_vmaa] = create_buffer((vk::BufferUsageFlagBits::eTransferSrc), VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, {vmc.queue_family_indices.transfer});
                void* mapped_data;
                vmaMapMemory(vmc.va, staging_vmaa, &mapped_data);
                memcpy(mapped_data, data, sizeof(T) * elements);
                vmaUnmapMemory(vmc.va, staging_vmaa);

                const vk::CommandBuffer& cb(vcc.begin(vcc.transfer_cb[0]));

                vk::BufferCopy copy_region{};
                copy_region.srcOffset = 0;
                copy_region.dstOffset = 0;
                copy_region.size = sizeof(T) * elements;
                cb.copyBuffer(staging_buffer, buffer, copy_region);
                vcc.submit_transfer(cb, true);

                vmaDestroyBuffer(vmc.va, staging_buffer, staging_vmaa);
            }
            else
            {
                void* mapped_mem;
                vmaMapMemory(vmc.va, vmaa, &mapped_mem);
                memcpy(mapped_mem, data, sizeof(T) * elements);
                vmaUnmapMemory(vmc.va, vmaa);
            }
        }

        template<class T>
        void update_data(const std::vector<T>& data)
        {
            update_data(data.data(), data.size());
        }

    private:
        std::pair<vk::Buffer, VmaAllocation> create_buffer(vk::BufferUsageFlags usage_flags, VmaAllocationCreateFlags vma_flags, const std::vector<uint32_t>& queue_family_indices)
        {
            vk::BufferCreateInfo bci{};
            bci.sType = vk::StructureType::eBufferCreateInfo;
            bci.size = byte_size;
            bci.usage = usage_flags;
            bci.sharingMode = queue_family_indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
            bci.flags = {};
            bci.queueFamilyIndexCount = queue_family_indices.size();
            bci.pQueueFamilyIndices = queue_family_indices.data();
            VmaAllocationCreateInfo vaci{};
            vaci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            vaci.flags = vma_flags;
            VkBuffer local_buffer;
            VmaAllocation local_vmaa;
            vmaCreateBuffer(vmc.va, (VkBufferCreateInfo*) (&bci), &vaci, (&local_buffer), &local_vmaa, nullptr);

            return std::make_pair(local_buffer, local_vmaa);
        }

        const VulkanMainContext& vmc;
        const VulkanCommandContext& vcc;
        bool device_local;
        uint64_t byte_size;
        uint64_t element_count;
        vk::Buffer buffer;
        VmaAllocation vmaa;
    };
} // namespace ve
