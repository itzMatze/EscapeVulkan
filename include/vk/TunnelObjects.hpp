#pragma once

#include <cstdint>
#include <queue>
#include <glm/vec3.hpp>
#include <boost/align/aligned_allocator.hpp>
#include <random>

#include "vk/Tunnel.hpp"
#include "vk/Fireflies.hpp"
#include "vk/PathTracer.hpp"

namespace ve
{
    struct NewSegmentPushConstants
    {
        alignas(16) glm::vec3 p0;
        alignas(16) glm::vec3 p1;
        alignas(16) glm::vec3 p2;
        uint32_t indices_start_idx;
        uint32_t segment_uid;
    };

    class TunnelObjects
    {
    public:
        TunnelObjects(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void self_destruct(bool full = true);
        void create_buffers(PathTracer& path_tracer);
        void construct(const RenderPass& render_pass);
        void restart(PathTracer& path_tracer);
        void reload_shaders(const RenderPass& render_pass);
        void draw(vk::CommandBuffer& cb, GameState& gs);
        // move tunnel one segment forward if player enters the n-th segment
        void advance(GameState& gs, DeviceTimer& timer, PathTracer& path_tracer);
        bool is_pos_past_segment(glm::vec3 pos, uint32_t idx, bool use_global_id);
        glm::vec3 get_player_reset_position();
        glm::vec3 get_player_reset_normal();

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        Fireflies fireflies;
        Tunnel tunnel;
        DescriptorSetHandler compute_dsh;
        std::vector<glm::vec3, boost::alignment::aligned_allocator<glm::vec3, 16>> tunnel_bezier_points;
        std::vector<uint32_t> blas_indices;
        std::vector<uint32_t> instance_indices;
        std::queue<glm::vec3> tunnel_bezier_points_queue;
        uint32_t tunnel_bezier_points_buffer;
        NewSegmentPushConstants cpc;
        Pipeline compute_pipeline;
        Pipeline compute_normals_pipeline;
        std::mt19937 rnd;
        std::uniform_real_distribution<float> dis;

        glm::vec3 random_cosine(const glm::vec3& normal, const float cosine_weight = 40.0f);
        void construct_pipelines();
        void init_tunnel(vk::CommandBuffer& cb, PathTracer& path_tracer);
        void compute_new_segment(vk::CommandBuffer& cb, uint32_t current_frame);
        glm::vec3 pop_tunnel_bezier_point_queue();
        glm::vec3& get_tunnel_bezier_point(uint32_t segment_id, uint32_t bezier_point_idx, bool use_global_id);
    };
} // namespace ve
