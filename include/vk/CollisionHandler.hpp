#pragma once

#include "VulkanMainContext.hpp"
#include "Storage.hpp"
#include "common.hpp"
#include "vk/Pipeline.hpp"
#include "vk/Timer.hpp"

namespace ve
{
    class CollisionHandler
    {
    public:
        CollisionHandler(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void create_buffers(const std::vector<Vertex>& vertices, uint32_t scene_player_start_idx, uint32_t scene_player_idx_count);
        void construct(const RenderPass& render_pass);
        void reload_shaders(const RenderPass& render_pass);
        void self_destruct(bool full = true);
        void draw(vk::CommandBuffer& cb, GameState& gs, const glm::mat4& mvp);
        void compute(GameState& gs, DeviceTimer& timer, PlayerTunnelCollisionPushConstants& ptcpc);
        int32_t get_shader_return_value(uint32_t frame_idx);
        void reset_shader_return_values(uint32_t frame_idx);
    private:
        struct BoundingBox
        {
            alignas(16) glm::vec3 min;
            alignas(16) glm::vec3 max;
        };

        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        uint32_t player_start_idx;
        uint32_t player_idx_count;
        BoundingBox bb;
        uint32_t bb_buffer;
        std::vector<uint32_t> return_buffers;
        uint32_t vertex_buffer;
        DescriptorSetHandler compute_dsh;
        Pipeline compute_pipeline;
        Pipeline render_pipeline;

        void construct_pipelines(const RenderPass& render_pass);
    };
} // namespace ve
