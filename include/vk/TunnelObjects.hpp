#pragma once

#include <cstdint>

#include "vk/Tunnel.hpp"
#include "vk/Fireflies.hpp"

namespace ve
{
    constexpr float segment_scale = 40.0f;
    constexpr uint32_t segment_count = 16; // how many segments are in the tunnel (must be power of two)
    static_assert((segment_count & (segment_count - 1)) == 0);
    constexpr uint32_t samples_per_segment = 64; // how many sample rings one segment is made of
    constexpr uint32_t vertices_per_sample = 360; // how many vertices are sampled in one sample ring
    constexpr uint32_t vertex_count = segment_count * samples_per_segment * vertices_per_sample;
    // two triangles per vertex on a sample (3 indices per triangle); every sample of a segment except the last one has triangles
    constexpr uint32_t indices_per_segment = (samples_per_segment - 1) * vertices_per_sample * 6;
    constexpr uint32_t index_count = indices_per_segment * segment_count;
    constexpr uint32_t fireflies_per_segment = 50;
    constexpr uint32_t firefly_count = fireflies_per_segment * segment_count;

    class TunnelObjects
    {
    public:
        TunnelObjects(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void self_destruct(bool full = true);
        void construct(const RenderPass& render_pass, uint32_t parallel_units);
        void reload_shaders(const RenderPass& render_pass);
        void draw(vk::CommandBuffer& cb, DrawInfo& di);
        // move tunnel one segment forward if player enters the n-th segment
        bool advance(const DrawInfo& di, DeviceTimer& timer);
    private:
        struct SegmentPlane
        {
            glm::vec3 pos;
            glm::vec3 normal;
        };

        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        Fireflies fireflies;
        Tunnel tunnel;
        DescriptorSetHandler compute_dsh;
        FixVector<SegmentPlane> segment_planes;
        NewSegmentPushConstants cpc;
        uint32_t tunnel_render_index_start = 0;
        Pipeline compute_pipeline;
        std::mt19937 rnd;
        std::uniform_real_distribution<float> dis;

        glm::vec3 random_cosine(const glm::vec3& normal);
        void construct_pipelines(const RenderPass& render_pass);
        void compute_new_segment(vk::CommandBuffer& cb, uint32_t current_frame);
    };
} // namespace ve
