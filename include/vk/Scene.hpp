#pragma once

#include "vk/RenderObject.hpp"
#include "vk/Model.hpp"
#include "common.hpp"

namespace ve
{
    class Scene
    {
    public:
        Scene(const VulkanMainContext& vmc);
        void construct_models(VulkanCommandContext& vcc);
        void construct_render_objects(const vk::RenderPass& render_pass);
        void self_destruct();
        void load(const std::string& path);
        void add_model(const std::string& key, ModelHandle model_handle);
        void add_bindings();
        Model* get_model(const std::string& key);
        DescriptorSetHandler& get_dsh(ShaderFlavor flavor);
        void draw(vk::CommandBuffer& cb, uint32_t current_frame, const glm::mat4& vp);
    private:
        std::unordered_map<ShaderFlavor, RenderObject> ros;
        std::unordered_map<std::string, ModelHandle> model_handles;
    };
}