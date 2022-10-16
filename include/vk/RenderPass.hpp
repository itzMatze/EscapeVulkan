#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class RenderPass
    {
    public:
        RenderPass(const VulkanMainContext& vmc, const vk::Format& color_format, const vk::Format& depth_format);
        const vk::RenderPass get() const;
        void self_destruct();
        
    private:
        const VulkanMainContext& vmc;
        vk::RenderPass render_pass;
    };
}// namespace ve
