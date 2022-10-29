#pragma once

#include "vk/Buffer.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk_mem_alloc.h"

namespace ve
{
    class Image
    {
    public:
        Image(const VulkanMainContext& vmc, const std::string& name, bool use_mip_maps);
        Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, const unsigned char* data, uint32_t width, uint32_t height, bool use_mip_maps);
        Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, const std::string& filename, bool use_mip_maps);
        void create_image(const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage, vk::Format format, uint32_t width, uint32_t height, vk::SampleCountFlagBits sample_count);
        void create_image(const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage, vk::Format format, vk::SampleCountFlagBits sample_count);
        void create_image_view(vk::Format format, vk::ImageAspectFlags aspects);
        void self_destruct();
        void transition_image_layout(vk::Format format, vk::ImageLayout new_layout, const VulkanCommandContext& vcc);
        vk::DeviceSize get_byte_size() const;
        vk::ImageView get_view() const;
        vk::Sampler get_sampler() const;

    private:
        const VulkanMainContext& vmc;
        const std::string name;
        int w, h, c;
        uint32_t mip_levels;
        vk::DeviceSize byte_size;
        vk::ImageLayout layout;
        vk::Image image;
        VmaAllocation vmaa;
        vk::ImageView view;
        vk::Sampler sampler;

        void create_image_from_data(const unsigned char* data, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices);
        void copy_buffer_to_image(const VulkanCommandContext& vcc, const Buffer& buffer);
        void generate_mipmaps(const VulkanCommandContext& vcc);
    };
}// namespace ve
