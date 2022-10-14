#pragma once

#include "vk/VulkanCommandContext.hpp"
#include "vk/Buffer.hpp"

namespace ve
{
    class Image
    {
    public:
        Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, const std::string& filename);
        void self_destruct();
        void transition_image_layout(vk::Format format, vk::ImageLayout new_layout, const VulkanCommandContext& vcc);
        vk::DeviceSize get_byte_size() const;
        vk::ImageView get_view() const;
        vk::Sampler get_sampler() const;

    private:
        const VulkanMainContext& vmc;
        const std::string name;
        int w, h, c;
        vk::DeviceSize byte_size;
        vk::ImageLayout layout;
        vk::Image image;
        vk::DeviceMemory memory;
        vk::ImageView view;
        vk::Sampler sampler;

        void copy_buffer_to_image(const VulkanCommandContext& vcc, const Buffer& buffer);
    };
}// namespace ve
