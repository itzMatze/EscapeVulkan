#pragma once

#include "common.hpp"
#include "vk/Model.hpp"
#include "vk/RenderObject.hpp"

namespace ve
{
    class Scene
    {
    public:
        Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc);
        void construct(const vk::RenderPass& render_pass);
        void self_destruct();
        void load(const std::string& path);
        void add_model(const std::string& key, ModelHandle model_handle);
        void add_bindings();
        Model* get_model(const std::string& key);
        DescriptorSetHandler& get_dsh(ShaderFlavor flavor);
        void draw(vk::CommandBuffer& cb, uint32_t current_frame, const glm::mat4& vp);

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        std::unordered_map<ShaderFlavor, RenderObject> ros;
        std::unordered_map<std::string, ModelHandle> model_handles;
        std::vector<Image> images;
        std::vector<Material> materials;
    };
}// namespace ve
