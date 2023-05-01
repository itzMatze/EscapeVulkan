#pragma once

#include "FixVector.hpp"
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
        void self_destruct();
        void add_bindings();
        void construct(const RenderPass& render_pass, uint32_t parallel_units);
        void draw(vk::CommandBuffer& cb, DrawInfo& di);
        void compute(vk::CommandBuffer& cb, const DrawInfo& di);
        // move tunnel one segment forward if player enters the n-th segment
        bool advance(const DrawInfo& di);

    private:
        struct SegmentPlane
        {
            glm::vec3 pos;
            glm::vec3 normal;
        };

        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        DescriptorSetHandler render_dsh;
        DescriptorSetHandler compute_dsh;
        uint32_t vertex_buffer;
        uint32_t index_buffer;
        ModelRenderData mrd;
        std::vector<uint32_t> model_render_data_buffers;
        FixVector<SegmentPlane> segment_planes;
        ComputePushConstants cpc;
        uint32_t render_index_start = 0;
        Pipeline pipeline;
        Pipeline mesh_view_pipeline;
        Pipeline compute_pipeline;
    };
} // namespace ve

