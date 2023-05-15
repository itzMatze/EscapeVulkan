#pragma once

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include "vk/Buffer.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk_mem_alloc.h"

namespace ve
{
    void blit_image(vk::CommandBuffer& cb, vk::Image& src, uint32_t src_mip_map_lvl, vk::Offset3D src_offset, vk::Image& dst, uint32_t dst_mip_map_lvl, vk::Offset3D dst_offset, uint32_t layer_count);
    void copy_image(vk::CommandBuffer& cb, vk::Image& src, vk::Image& dst, uint32_t width, uint32_t height, uint32_t layer_count);
    void perform_image_layout_transition(vk::CommandBuffer& cb, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::PipelineStageFlags src_stage_flags, vk::PipelineStageFlags dst_stage_flags, vk::AccessFlags src_access_flags, vk::AccessFlags dst_access_flags, uint32_t base_mip_level, uint32_t mip_levels, uint32_t layer_count);

    class Image
    {
    public:
        // used to create texture from raw data
        Image(const VulkanMainContext& vmc, VulkanCommandContext& vcc, const unsigned char* data, uint32_t width, uint32_t height, bool use_mip_maps, uint32_t base_mip_map_lvl, const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage_flags) : vmc(vmc), w(width), h(height), c(4), byte_size(width * height * 4), mip_levels(use_mip_maps ? std::floor(std::log2(std::max(w, h))) + 1 : 1), layer_count(1)
        {
            create_image_from_data(data, vcc, queue_family_indices, base_mip_map_lvl, usage_flags);
        }

        // used to create texture array from raw data
        Image(const VulkanMainContext& vmc, VulkanCommandContext& vcc, const std::vector<std::vector<unsigned char>>& data, uint32_t width, uint32_t height, bool use_mip_maps, uint32_t base_mip_map_lvl, const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage_flags) : vmc(vmc), w(width), h(height), c(4), byte_size(width * height * 4 * data.size()), mip_levels(use_mip_maps ? std::floor(std::log2(std::max(w, h))) + 1 : 1), layer_count(data.size())
        {
            std::vector<unsigned char> copy_data;
            for (const auto& i : data)
            {
                for (const auto& j : i) copy_data.push_back(j);
            }
            create_image_from_data(copy_data.data(), vcc, queue_family_indices, base_mip_map_lvl, usage_flags);
        }

        // used to create texture from file
        Image(const VulkanMainContext& vmc, VulkanCommandContext& vcc, const std::string& filename, bool use_mip_maps, uint32_t base_mip_map_lvl, const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage_flags) : vmc(vmc), layer_count(1)
        {
            stbi_uc* pixels = stbi_load(filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
            VE_ASSERT(pixels, "Failed to load image \"{}\"!", filename);
            byte_size = w * h * 4;
            mip_levels = use_mip_maps ? std::floor(std::log2(std::max(w, h))) + 1 : 1;
            create_image_from_data(pixels, vcc, queue_family_indices, base_mip_map_lvl, usage_flags);
            stbi_image_free(pixels);
            pixels = nullptr;
        }

        // used to create depth buffer and multisampling color attachment
        Image(const VulkanMainContext& vmc, uint32_t width, uint32_t height, vk::ImageUsageFlags usage, vk::Format format, vk::SampleCountFlagBits sample_count, bool use_mip_maps, uint32_t base_mip_map_lvl, const std::vector<uint32_t>& queue_family_indices, bool image_view_required = true) : vmc(vmc), format(format), w(width), h(height), c(4), mip_levels(use_mip_maps ? std::floor(std::log2(std::max(w, h))) + 1 : 1), layer_count(1)
        {
            std::tie(image, vmaa) = create_image(queue_family_indices, usage, sample_count, use_mip_maps, format, vk::Extent3D(w, h, 1), layer_count, vmc.va, !image_view_required);
            layout = vk::ImageLayout::eUndefined;
            if(image_view_required) create_image_view(usage & vk::ImageUsageFlagBits::eDepthStencilAttachment ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor);
        }

        void self_destruct();
        void transition_image_layout(VulkanCommandContext& vcc, vk::ImageLayout new_layout, vk::PipelineStageFlags src_stage_flags, vk::PipelineStageFlags dst_stage_flags, vk::AccessFlags src_access_flags, vk::AccessFlags dst_access_flags);
        void save_to_file();
        vk::DeviceSize get_byte_size() const;
        uint32_t get_layer_count() const;
        vk::ImageLayout get_layout() const;
        vk::Image& get_image();
        vk::ImageView& get_view();
        vk::Sampler& get_sampler();

    private:
        const VulkanMainContext& vmc;
        vk::Format format = vk::Format::eR8G8B8A8Unorm;
        int w, h, c;
        uint32_t mip_levels;
        uint32_t layer_count;
        vk::DeviceSize byte_size;
        vk::ImageLayout layout;
        vk::Image image;
        VmaAllocation vmaa;
        vk::ImageView view;
        vk::Sampler sampler;

        static std::pair<vk::Image, VmaAllocation> create_image(const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage, vk::SampleCountFlagBits sample_count, bool use_mip_levels, vk::Format format, vk::Extent3D extent, uint32_t layer_count, const VmaAllocator& va, bool host_visible = false);
        void create_image_from_data(const unsigned char* data, VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, uint32_t base_mip_map_lvl, vk::ImageUsageFlags usage_flags);
        void create_image_view(vk::ImageAspectFlags aspects);
        void create_sampler();
        void generate_mipmaps(VulkanCommandContext& vcc);
    };
} // namespace ve
