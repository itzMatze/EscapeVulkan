#pragma once

#include "vk/common.hpp"
#include "vk/Model.hpp"
#include "vk/RenderObject.hpp"
#include "Storage.hpp"

namespace ve
{
    class Scene
    {
    public:
        Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void construct(const RenderPass& render_pass, uint32_t parallel_units);
        void self_destruct();
        void load(const std::string& path);
        void translate(const std::string& model, const glm::vec3& trans);
        void scale(const std::string& model, const glm::vec3& scale);
        void rotate(const std::string& model, float degree, const glm::vec3& axis);
        DescriptorSetHandler& get_dsh(ShaderFlavor flavor);
        void draw(vk::CommandBuffer& cb, DrawInfo& di);

        bool loaded = false;

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        std::unordered_map<ShaderFlavor, RenderObject> ros;
        std::vector<Light> lights;
        std::vector<std::pair<glm::vec3, glm::vec3>> initial_light_values;
        std::vector<Material> materials;
        std::vector<std::vector<unsigned char>> texture_data;
        std::vector<ModelRenderData> model_render_data;
        std::unordered_map<std::string, uint32_t> model_handles;
        uint32_t vertex_buffer;
        uint32_t index_buffer;
        // use -1 to encode missing material buffer and/or textures as they are not required
        int32_t material_buffer = -1;
        int32_t texture_image = -1;
        int32_t light_buffer = -1;
        std::vector<uint32_t> model_render_data_buffers;

        void add_model(Model& model, const std::string& name);
    };
} // namespace ve
