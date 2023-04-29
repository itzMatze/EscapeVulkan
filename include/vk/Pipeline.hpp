#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/DescriptorSetHandler.hpp"
#include "vk/RenderPass.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/Shader.hpp"

namespace ve
{
    class Pipeline
    {
    public:
        Pipeline(const VulkanMainContext& vmc);
        void self_destruct();
        void construct(const RenderPass& render_pass, vk::DescriptorSetLayout set_layout, const std::vector<ShaderInfo>& shader_infos, vk::PolygonMode polygon_mode);
        void construct(vk::DescriptorSetLayout set_layout, const ShaderInfo& shader_info);
        const vk::Pipeline& get() const;
        const vk::PipelineLayout& get_layout() const;

    private:
        const VulkanMainContext& vmc;
        vk::PipelineLayout pipeline_layout;
        vk::Pipeline pipeline;
    };
} // namespace ve
