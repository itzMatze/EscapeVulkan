#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class RenderPass
    {
    public:
        RenderPass(const VulkanMainContext& vmc, const vk::Format& color_format, const vk::Format& depth_format, vk::SampleCountFlagBits sample_count);
        vk::RenderPass get() const;
        vk::SampleCountFlagBits get_sample_count() const;
        void self_destruct();
        
    private:
        const VulkanMainContext& vmc;
        vk::SampleCountFlagBits sample_count;
        vk::RenderPass render_pass;
    };
}// namespace ve
