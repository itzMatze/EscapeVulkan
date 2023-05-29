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
        Mesh(int32_t material_idx, uint32_t idx_offset, uint32_t idx_count);
        void draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, const std::vector<vk::DescriptorSet>& sets, GameState& gs);

    private:
        uint32_t index_offset, index_count;
        int32_t mat_idx;
    };
} // namespace ve
