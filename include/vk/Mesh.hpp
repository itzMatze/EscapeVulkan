#pragma once

#include "vk/DescriptorSetHandler.hpp"
#include "vk/Image.hpp"
#include "vk/common.hpp"

namespace ve
{
    class Mesh
    {
    public:
        Mesh(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<std::string>& texture_names, DescriptorSetHandler& dsh, const std::unordered_map<std::string, Image>& textures);
        void self_destruct();
        void draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, uint32_t current_frame);

    private:
        Buffer vertex_buffer;
        Buffer index_buffer;
        DescriptorSetHandler& dsh;
        uint32_t descriptor_set_start_idx;
        const std::vector<std::string> texture_names;
    };
}// namespace ve
