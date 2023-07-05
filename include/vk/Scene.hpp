#pragma once

#include "vk/common.hpp"
#include "vk/Model.hpp"
#include "vk/RenderObject.hpp"
#include "Storage.hpp"
#include "Timer.hpp"
#include "TunnelObjects.hpp"
#include "CollisionHandler.hpp"
#include "vk/PathTracer.hpp"

namespace ve
{
    class Scene
    {
    public:
        Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void construct(const RenderPass& render_pass);
        void self_destruct();
        void reload_shaders(const RenderPass& render_pass);
        void load(const std::string& path);
        void translate(const std::string& model, const glm::vec3& trans);
        void scale(const std::string& model, const glm::vec3& scale);
        void rotate(const std::string& model, float degree, const glm::vec3& axis);
        DescriptorSetHandler& get_dsh(ShaderFlavor flavor);
        void draw(vk::CommandBuffer& cb, GameState& gs, DeviceTimer& timer);
        void update_game_state(vk::CommandBuffer& cb, GameState& gs, DeviceTimer& timer);

        bool loaded = false;

    private:
        struct ModelInfo {
            uint32_t index_buffer_idx;
            uint32_t num_indices;
            uint32_t blas_idx;
            uint32_t instance_idx;
        };

        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        std::unordered_map<ShaderFlavor, RenderObject> ros;
        std::vector<Light> lights;
        std::vector<std::pair<glm::vec3, glm::vec3>> initial_light_values;
        std::vector<ModelRenderData> model_render_data;
        std::unordered_map<std::string, uint32_t> model_handles;
        std::vector<ModelInfo> model_infos;
        std::vector<uint32_t> bb_mm_buffers;
        uint32_t vertex_buffer;
        uint32_t index_buffer;
        // use -1 to encode missing material buffer and/or textures as they are not required
        int32_t material_buffer = -1;
        int32_t texture_image = -1;
        std::array<int32_t, 2> light_buffers = {-1, -1};
        std::vector<uint32_t> model_render_data_buffers;
        TunnelObjects tunnel_objects;
        CollisionHandler collision_handler;
        PathTracer path_tracer;

        void construct_pipelines(const RenderPass& render_pass, bool reload);
    };
} // namespace ve
