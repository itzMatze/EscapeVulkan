#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/DescriptorSetHandler.hpp"
#include "vk/RenderPass.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/Shader.hpp"
#include "vk/common.hpp"

namespace ve
{
    class Pipeline
    {
    public:
        Pipeline(const VulkanMainContext& vmc);
        void self_destruct();
        void construct(const RenderPass& render_pass, vk::DescriptorSetLayout set_layout, const std::vector<ShaderInfo>& shader_infos, vk::PolygonMode polygon_mode, const std::vector<vk::VertexInputBindingDescription>& binding_descriptions = Vertex::get_binding_descriptions(), const std::vector<vk::VertexInputAttributeDescription>& attribute_description = Vertex::get_attribute_descriptions(), const vk::PrimitiveTopology& primitive_topology = vk::PrimitiveTopology::eTriangleList);
        void construct(vk::DescriptorSetLayout set_layout, const ShaderInfo& shader_info, uint32_t push_constant_byte_size);
        const vk::Pipeline& get() const;
        const vk::PipelineLayout& get_layout() const;

    private:
        const VulkanMainContext& vmc;
        vk::PipelineLayout pipeline_layout;
        vk::Pipeline pipeline;
    };
} // namespace ve
