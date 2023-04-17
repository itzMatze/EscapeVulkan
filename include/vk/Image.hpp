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
        Image(const VulkanMainContext& vmc, const std::string& name, bool use_mip_maps) : vmc(vmc), name(name), mip_levels(use_mip_maps ? 2 : 1)
        {}

        template<class... Args> Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const unsigned char* data, uint32_t width, uint32_t height, bool use_mip_maps, Args... queue_family_indices) : vmc(vmc), w(width), h(height), byte_size(width * height * 4), mip_levels(use_mip_maps ? 2 : 1)
        {
            std::vector<uint32_t> queue_family_indices_vec = {queue_family_indices...};
            create_image_from_data(data, vcc, queue_family_indices_vec);
        }

        template<class... Args> Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, Image& src_image, uint32_t base_mip_level, Args... queue_family_indices) : vmc(vmc), mip_levels(2)
        {
            std::vector<uint32_t> queue_family_indices_vec = {queue_family_indices...};
            w = std::max(1.0, src_image.w / std::pow(2, base_mip_level));
            h = std::max(1.0, src_image.h / std::pow(2, base_mip_level));
            byte_size = w * h * 4;
            constexpr vk::Format format = vk::Format::eR8G8B8A8Unorm;
            create_image(queue_family_indices_vec, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, format, vk::SampleCountFlagBits::e1);

            src_image.transition_image_layout(vcc, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead);
            transition_image_layout(vcc, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, vk::AccessFlagBits::eTransferWrite);
            vk::FormatProperties format_properties = vmc.physical_device.get().getFormatProperties(format);
            if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) mip_levels = 1;

            const vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[0]);

            vk::ImageBlit blit{};
            blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.srcOffsets[1] = vk::Offset3D(w, h, 1);
            blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.mipLevel = base_mip_level;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.dstOffsets[1] = vk::Offset3D(w, h, 1);
            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.mipLevel = 0;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            cb.blitImage(src_image.get_image(), vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

            vcc.submit_graphics(cb, true);

            mip_levels > 1 ? generate_mipmaps(vcc) : transition_image_layout(vcc, vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead);
            create_image_view(format, vk::ImageAspectFlagBits::eColor);
            create_sampler();
        }

        template<class... Args> Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::string& filename, bool use_mip_maps, Args... queue_family_indices) : vmc(vmc), name(filename), mip_levels(use_mip_maps ? 2 : 1)
        {
            std::vector<uint32_t> queue_family_indices_vec = {queue_family_indices...};
            stbi_uc* pixels = stbi_load(filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
            VE_ASSERT(pixels, "Failed to load image \"{}\"!", filename);
            byte_size = w * h * 4;
            create_image_from_data(pixels, vcc, queue_family_indices_vec);
            stbi_image_free(pixels);
            pixels = nullptr;
        }

        void create_image(const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage, vk::Format format, uint32_t width, uint32_t height, vk::SampleCountFlagBits sample_count);
        void create_image(const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage, vk::Format format, vk::SampleCountFlagBits sample_count);
        void create_image_view(vk::Format format, vk::ImageAspectFlags aspects);
        void create_sampler();
        void self_destruct();
        void transition_image_layout(const VulkanCommandContext& vcc, vk::ImageLayout new_layout, vk::PipelineStageFlags src_stage_flags, vk::PipelineStageFlags dst_stage_flags, vk::AccessFlags src_access_flags, vk::AccessFlags dst_access_flags);
        vk::DeviceSize get_byte_size() const;
        vk::Image get_image() const;
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
} // namespace ve
