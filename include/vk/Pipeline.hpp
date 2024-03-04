#pragma once

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
        void construct(const RenderPass& render_pass, std::optional<vk::DescriptorSetLayout> set_layout, const std::vector<ShaderInfo>& shader_infos, vk::PolygonMode polygon_mode, const std::vector<vk::VertexInputBindingDescription>& binding_descriptions, const std::vector<vk::VertexInputAttributeDescription>& attribute_description, const vk::PrimitiveTopology& primitive_topology, const std::vector<vk::PushConstantRange>& pcrs);
        void construct(vk::DescriptorSetLayout set_layout, const ShaderInfo& shader_info, uint32_t push_constant_byte_size);
        const vk::Pipeline& get() const;
        const vk::PipelineLayout& get_layout() const;

    private:
        const VulkanMainContext& vmc;
        vk::PipelineLayout pipeline_layout;
        vk::Pipeline pipeline;
    };
} // namespace ve
