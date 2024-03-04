#pragma once

#include "vk/Pipeline.hpp"
#include "Storage.hpp"
#include "vk/Mesh.hpp"
#include "vk/common.hpp"

namespace ve
{
    class JetParticles
    {
    public:
        JetParticles(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void self_destruct(bool full = true);
        void create_buffers();
        void construct(const RenderPass& render_pass, const Mesh& spawn_mesh, const std::vector<uint32_t>& spawn_mesh_model_render_data_buffer, uint32_t spawn_mesh_model_render_data_idx);
        void reload_shaders(const RenderPass& render_pass);
        void draw(vk::CommandBuffer& cb, GameState& gs);
        void move_step(vk::CommandBuffer& cb, const GameState& gs);

        std::vector<uint32_t> vertex_buffers;

    private:
        static constexpr float max_particle_lifetime = 0.3f;
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        DescriptorSetHandler render_dsh;
        DescriptorSetHandler compute_dsh;
        ModelRenderData mrd;
        std::vector<uint32_t> model_render_data_buffers;
        uint32_t spawn_mesh_model_render_data_buffer_count;
        uint32_t spawn_mesh_model_render_data_buffer_idx;
        Pipeline render_pipeline;
        Pipeline move_compute_pipeline;
        Mesh mesh;
        
        void construct_pipelines(const RenderPass& render_pass);
    };
} // namespace ve

