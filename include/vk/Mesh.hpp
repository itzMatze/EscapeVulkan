#pragma once

#include "vk/DescriptorSetHandler.hpp"
#include "vk/Image.hpp"
#include "vk/common.hpp"
#include "Storage.hpp"

namespace ve
{
    class Mesh
    {
    public:
        Mesh() = default;
        Mesh(int32_t material_idx, uint32_t idx_offset, uint32_t idx_count, const std::string& name);
        void draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, const std::vector<vk::DescriptorSet>& sets, GameState& gs);

        int32_t material_idx;
        uint32_t mesh_render_data_idx;
        uint32_t index_offset;
        uint32_t index_count;
        std::string name;
    };
} // namespace ve
