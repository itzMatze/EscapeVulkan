#pragma once

#include "vk/Timer.hpp"
#include "vk/Pipeline.hpp"
#include "Storage.hpp"
#include "vk/common.hpp"
#include "vk/DescriptorSetHandler.hpp"

namespace ve
{
    class Fireflies
    {
    public:
        Fireflies(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void self_destruct(bool full = true);
        void create_buffers();
        void construct(const RenderPass& render_pass);
        void reload_shaders(const RenderPass& render_pass);
        void draw(vk::CommandBuffer& cb, GameState& gs);
        void move_step(vk::CommandBuffer& cb, const GameState& gs, DeviceTimer& timer, uint32_t segment_uid);

        std::vector<uint32_t> vertex_buffers;

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        DescriptorSetHandler render_dsh;
        DescriptorSetHandler compute_dsh;
        ModelRenderData mrd;
        std::vector<uint32_t> model_render_data_buffers;
        Pipeline render_pipeline;
        Pipeline move_compute_pipeline;
        Pipeline tunnel_collision_compute_pipeline;
        
        void construct_pipelines(const RenderPass& render_pass);
    };
} // namespace ve
