#pragma once

#include "vk/Pipeline.hpp"
#include "vk/Scene.hpp"
#include "vk/common.hpp"

namespace ve
{
    class RenderObject
    {
    public:
        RenderObject(const VulkanMainContext& vmc);
        void self_destruct();
        void add_scene(VulkanCommandContext& vcc, const std::string& path, const glm::mat4& transformation);
        void add_scene(VulkanCommandContext& vcc, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const Material* material, const glm::mat4& transformation);
        void add_bindings();
        void construct(const vk::RenderPass render_pass, const std::vector<std::pair<std::string, vk::ShaderStageFlagBits>>& shader_names, vk::PolygonMode polygon_mode);
        void draw(vk::CommandBuffer& cb, uint32_t current_frame, const glm::mat4& vp);
        void change_transformation(uint32_t idx, const glm::mat4& trans);
        void set_transformation(uint32_t idx, const glm::mat4& trans);

        DescriptorSetHandler dsh;

    private:
        const VulkanMainContext& vmc;
        std::vector<Scene> scenes;
        Pipeline pipeline;
    };
}// namespace ve
