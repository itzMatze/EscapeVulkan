#pragma once

#include "vk/DescriptorSetHandler.hpp"
#include "vk/Image.hpp"
#include "vk/common.hpp"

namespace ve
{
    class Mesh
    {
    public:
        Mesh(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const Material* material);
        void self_destruct();
        void add_set_bindings(DescriptorSetHandler& dsh);
        void draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, const std::vector<vk::DescriptorSet>& sets, uint32_t current_frame);

    private:
        Buffer vertex_buffer;
        Buffer index_buffer;
        std::vector<uint32_t> descriptor_set_indices;
        const Material* mat;
    };
}// namespace ve
