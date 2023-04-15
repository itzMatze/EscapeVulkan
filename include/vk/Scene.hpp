#pragma once

#include "vk/common.hpp"
#include "vk/Model.hpp"
#include "vk/RenderObject.hpp"

namespace ve
{
    class Scene
    {
    public:
        Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc);
        void construct(const RenderPass& render_pass);
        void self_destruct();
        void load(const std::string& path);
        void add_bindings();
        void translate(const std::string& model, const glm::vec3& trans);
        void scale(const std::string& model, const glm::vec3& scale);
        void rotate(const std::string& model, float degree, const glm::vec3& axis);
        DescriptorSetHandler& get_dsh(ShaderFlavor flavor);
        void draw(vk::CommandBuffer& cb, DrawInfo& di);

        bool loaded = false;

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        std::unordered_map<ShaderFlavor, RenderObject> ros;
        std::vector<Model> models;
        std::unordered_map<std::string, uint32_t> model_handles;
    };
}// namespace ve
