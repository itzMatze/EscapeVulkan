#pragma once

#include "vk/Timer.hpp"
#include "vk/Pipeline.hpp"
#include "Storage.hpp"
#include "vk/common.hpp"

namespace ve
{
    class Fireflies
    {
    public:
        Fireflies(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void self_destruct(bool full = true);
        void construct(const RenderPass& render_pass);
        void reload_shaders(const RenderPass& render_pass);
        void draw(vk::CommandBuffer& cb, DrawInfo& di);
        void move_step(vk::CommandBuffer& cb, const DrawInfo& di, DeviceTimer& timer);
    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        DescriptorSetHandler render_dsh;
        DescriptorSetHandler compute_dsh;
        std::vector<uint32_t> vertex_buffers;
        ModelRenderData mrd;
        std::vector<uint32_t> model_render_data_buffers;
        Pipeline pipeline;
        Pipeline compute_pipeline;
        
        void construct_pipelines(const RenderPass& render_pass);
    };
} // namespace ve
