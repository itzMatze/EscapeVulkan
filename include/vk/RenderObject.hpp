#pragma once

#include "vk/Model.hpp"
#include "vk/Pipeline.hpp"
#include "vk/RenderPass.hpp"
#include "vk/common.hpp"

namespace ve
{
    // groups rendering of models that use the same pipeline together
    class RenderObject
    {
    public:
        RenderObject(const VulkanMainContext& vmc);
        void self_destruct();
        void add_model(uint32_t idx);
        void add_bindings(std::vector<Model>& models);
        void construct(const RenderPass& render_pass, const std::vector<std::pair<std::string, vk::ShaderStageFlagBits>>& shader_names, vk::PolygonMode polygon_mode);
        void draw(vk::CommandBuffer& cb, uint32_t current_frame, const glm::mat4& vp, std::vector<Model>& models);

        DescriptorSetHandler dsh;

    private:
        const VulkanMainContext& vmc;
        std::vector<uint32_t> model_indices;
        Pipeline pipeline;
    };
} // namespace ve
