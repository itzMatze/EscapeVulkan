#pragma once

#include "vk/common.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class RenderPass
    {
    public:
        RenderPass(const VulkanMainContext& vmc, const vk::Format& color_format, const vk::Format& depth_format);
        RenderPass(const VulkanMainContext& vmc, const vk::Format& depth_format);
        vk::RenderPass get() const;
        void self_destruct();

        uint32_t attachment_count;
        
    private:
        const VulkanMainContext& vmc;
        vk::RenderPass render_pass;
    };
} // namespace ve
