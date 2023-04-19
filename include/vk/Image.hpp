#pragma once

#include <stb/stb_image.h>

#include "vk/Buffer.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk_mem_alloc.h"

namespace ve
{
    class Image
    {
    public:
        // used to create texture from raw data
        Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const unsigned char* data, uint32_t width, uint32_t height, bool use_mip_maps, uint32_t base_mip_map_lvl, const std::vector<uint32_t>& queue_family_indices) : vmc(vmc), w(width), h(height), byte_size(width * height * 4), mip_levels(use_mip_maps ? std::floor(std::log2(std::max(w, h))) + 1 : 1)
        {
            create_image_from_data(data, vcc, queue_family_indices, base_mip_map_lvl);
        }

        // used to create texture from file
        Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::string& filename, bool use_mip_maps, uint32_t base_mip_map_lvl, const std::vector<uint32_t>& queue_family_indices) : vmc(vmc)
        {
            stbi_uc* pixels = stbi_load(filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
            VE_ASSERT(pixels, "Failed to load image \"{}\"!", filename);
            byte_size = w * h * 4;
            mip_levels = use_mip_maps ? std::floor(std::log2(std::max(w, h))) + 1 : 1;
            create_image_from_data(pixels, vcc, queue_family_indices, base_mip_map_lvl);
            stbi_image_free(pixels);
            pixels = nullptr;
        }

        // used to create depth buffer and multisampling color attachment
        Image(const VulkanMainContext& vmc, uint32_t width, uint32_t height, vk::ImageUsageFlags usage, vk::Format format, vk::SampleCountFlagBits sample_count, bool use_mip_maps, uint32_t base_mip_map_lvl, const std::vector<uint32_t>& queue_family_indices) : vmc(vmc), format(format), w(width), h(height), mip_levels(use_mip_maps ? std::floor(std::log2(std::max(w, h))) + 1 : 1)
        {
            std::tie(image, vmaa) = create_image(queue_family_indices, usage, sample_count, use_mip_maps, format, vk::Extent3D(w, h, 1), vmc.va);
            create_image_view(usage & vk::ImageUsageFlagBits::eDepthStencilAttachment ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor);
        }

        void self_destruct();
        void transition_image_layout(const VulkanCommandContext& vcc, vk::ImageLayout new_layout, vk::PipelineStageFlags src_stage_flags, vk::PipelineStageFlags dst_stage_flags, vk::AccessFlags src_access_flags, vk::AccessFlags dst_access_flags);
        vk::DeviceSize get_byte_size() const;
        vk::Image get_image() const;
        vk::ImageView get_view() const;
        vk::Sampler get_sampler() const;

    private:
        const VulkanMainContext& vmc;
        vk::Format format = vk::Format::eR8G8B8A8Unorm;
        int w, h, c;
        uint32_t mip_levels;
        vk::DeviceSize byte_size;
        vk::ImageLayout layout;
        vk::Image image;
        VmaAllocation vmaa;
        vk::ImageView view;
        vk::Sampler sampler;

        static std::pair<vk::Image, VmaAllocation> create_image(const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage, vk::SampleCountFlagBits sample_count, bool use_mip_levels, vk::Format format, vk::Extent3D extent, const VmaAllocator& va);
        void create_image_from_data(const unsigned char* data, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, uint32_t base_mip_map_lvl);
        void create_image_view(vk::ImageAspectFlags aspects);
        void create_sampler();
        void generate_mipmaps(const VulkanCommandContext& vcc);
    };
} // namespace ve
