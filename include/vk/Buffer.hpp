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

        template<class T>
        Buffer(const VulkanMainContext& vmc, const T* data, std::size_t elements, vk::BufferUsageFlags usage_flags, const std::vector<uint32_t>& queue_family_indices) : vmc(&vmc), device_local(false), element_count(elements), byte_size(sizeof(T) * elements)
        {
            std::tie(buffer, vmaa) = create_buffer(usage_flags, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, queue_family_indices);
            update_data(data, elements);
        }

        template<class T>
        Buffer(const VulkanMainContext& vmc, const std::vector<T>& data, vk::BufferUsageFlags usage_flags, const std::vector<uint32_t>& queue_family_indices) : Buffer(vmc, data.data(), data.size(), usage_flags, queue_family_indices)
        {}

        template<class T>
        Buffer(const VulkanMainContext& vmc, const std::vector<T>& data, vk::BufferUsageFlags usage_flags, std::vector<uint32_t> queue_family_indices, const VulkanCommandContext& vcc) : vmc(&vmc), device_local(true), element_count(data.size()), byte_size(sizeof(T) * data.size())
        {
            std::tie(buffer, vmaa) = create_buffer((usage_flags | vk::BufferUsageFlagBits::eTransferDst), {}, queue_family_indices);
            update_data(data, vcc);
        }

        void self_destruct()
        {
            vmaDestroyBuffer(vmc->va, buffer, vmaa);
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
            VE_ASSERT(!device_local, "Trying to update data to a buffer that is device local but it should not!");

            void* mapped_mem;
            vmaMapMemory(vmc->va, vmaa, &mapped_mem);
            memcpy(mapped_mem, data, sizeof(T) * elements);
            vmaUnmapMemory(vmc->va, vmaa);
        }

        template<class T>
        void update_data(const std::vector<T>& data)
        {
            update_data(data.data(), data.size());
        }

        template<class T>
        void update_data(const T& data, const VulkanCommandContext& vcc)
        {
            update_data(std::vector<T>{data}, vcc);
        }

        template<class T>
        void update_data(const std::vector<T>& data, const VulkanCommandContext& vcc)
        {
            VE_ASSERT(sizeof(T) * data.size() <= byte_size, "Data is larger than buffer!");
            VE_ASSERT(device_local, "Trying to update data to a buffer that is not device local but it should!");

            auto [staging_buffer, staging_vmaa] = create_buffer((vk::BufferUsageFlagBits::eTransferSrc), VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, {uint32_t(vmc->queues_family_indices.transfer)});
            void* mapped_data;
            vmaMapMemory(vmc->va, staging_vmaa, &mapped_data);
            memcpy(mapped_data, data.data(), sizeof(T) * data.size());
            vmaUnmapMemory(vmc->va, staging_vmaa);

            const vk::CommandBuffer& cb(vcc.begin(vcc.transfer_cb[0]));

            vk::BufferCopy copy_region{};
            copy_region.srcOffset = 0;
            copy_region.dstOffset = 0;
            copy_region.size = sizeof(T) * data.size();
            cb.copyBuffer(staging_buffer, buffer, copy_region);
            vcc.submit_transfer(cb, true);

            vmaDestroyBuffer(vmc->va, staging_buffer, staging_vmaa);
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
            vmaCreateBuffer(vmc->va, (VkBufferCreateInfo*) (&bci), &vaci, (&local_buffer), &local_vmaa, nullptr);

            return std::make_pair(local_buffer, local_vmaa);
        }

        const VulkanMainContext* vmc;
        bool device_local;
        uint64_t byte_size;
        uint64_t element_count;
        vk::Buffer buffer;
        VmaAllocation vmaa;
    };
}// namespace ve