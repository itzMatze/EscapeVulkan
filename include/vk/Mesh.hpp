#pragma once

#include "vk/DescriptorSetHandler.hpp"
#include "vk/Image.hpp"
#include "vk/common.hpp"

namespace ve
{
    class Mesh
    {
    public:
        Mesh(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const Material& material, uint32_t idx_offset, uint32_t idx_count);
        void self_destruct();
        void add_set_bindings(DescriptorSetHandler& dsh, VulkanStorageContext& vsc);
        void draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, const std::vector<vk::DescriptorSet>& sets, uint32_t current_frame);

    private:
        uint32_t index_offset, index_count;
        std::vector<uint32_t> descriptor_set_indices;
        const Material& mat;
    };
} // namespace ve
