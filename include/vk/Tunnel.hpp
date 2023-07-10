#pragma once

#include <random>
#include <cmath>

#include "FixVector.hpp"
#include "vk/Timer.hpp"
#include "vk/Model.hpp"
#include "vk/Pipeline.hpp"
#include "vk/RenderPass.hpp"
#include "vk/common.hpp"

namespace ve
{
    class Tunnel
    {
    public:
        Tunnel(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void self_destruct(bool full = true);
        void create_buffers();
        void construct(const RenderPass& render_pass);
        void reload_shaders(const RenderPass& render_pass);
        void draw(vk::CommandBuffer& cb, GameState& gs, const glm::vec3& p1, const glm::vec3& p2);

        uint32_t vertex_buffer;
        uint32_t index_buffer;

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        DescriptorSetHandler skybox_dsh;
        DescriptorSetHandler render_dsh;
        uint32_t skybox_vertex_buffer;
        ModelRenderData mrd;
        std::vector<uint32_t> model_render_data_buffers;
        uint32_t noise_textures;
        uint32_t skybox_texture;
        Pipeline skybox_render_pipeline;
        Pipeline pipeline;
        Pipeline mesh_view_pipeline;

        void construct_pipelines(const RenderPass& render_pass);
        void create_noise_textures();
    };
} // namespace ve

