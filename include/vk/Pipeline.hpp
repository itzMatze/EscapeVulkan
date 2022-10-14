#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/DescriptorSetHandler.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class Pipeline
    {
    public:
        Pipeline(const VulkanMainContext& vmc);
        void self_destruct();
        void construct(const vk::RenderPass& render_pass, const DescriptorSetHandler& ds_handler);
        const vk::Pipeline& get() const;
        const vk::PipelineLayout& get_layout() const;

    private:
        const VulkanMainContext& vmc;
        vk::PipelineLayout pipeline_layout;
        vk::Pipeline pipeline;
    };
}// namespace ve
