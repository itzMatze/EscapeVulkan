#pragma once

#include <cstdint>
#include <queue>
#include <glm/vec3.hpp>
#include <boost/align/aligned_allocator.hpp>

#include "vk/Tunnel.hpp"
#include "vk/Fireflies.hpp"
#include "vk/PathTracer.hpp"

namespace ve
{
    constexpr float segment_scale = 20.0f;
    constexpr uint32_t segment_count = 16; // how many segments are in the tunnel (must be power of two)
    static_assert((segment_count & (segment_count - 1)) == 0);
    constexpr uint32_t samples_per_segment = 32; // how many sample rings one segment is made of
    constexpr uint32_t vertices_per_sample = 360; // how many vertices are sampled in one sample ring
    constexpr uint32_t vertex_count = segment_count * samples_per_segment * vertices_per_sample;
    // two triangles per vertex on a sample (3 indices per triangle); every sample of a segment except the last one has triangles
    constexpr uint32_t indices_per_segment = (samples_per_segment - 1) * vertices_per_sample * 6;
    constexpr uint32_t index_count = indices_per_segment * segment_count;
    constexpr uint32_t fireflies_per_segment = 15;
    constexpr uint32_t firefly_count = fireflies_per_segment * segment_count;
    constexpr uint32_t jet_particle_count = 20000;
    constexpr uint32_t reservoir_count = 4;
    constexpr uint32_t player_local_segment_position = 2;

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
