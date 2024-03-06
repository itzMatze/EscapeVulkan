#include "vk/Mesh.hpp"

namespace ve
{
    Mesh::Mesh(int32_t material_idx, uint32_t idx_offset, uint32_t idx_count, const std::string& name) : material_idx(material_idx), index_offset(idx_offset), index_count(idx_count), name(name)
    {}

    void Mesh::draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, const std::vector<vk::DescriptorSet>& sets, GameState& gs)
    {
        RenderPushConstants pc{.mesh_render_data_idx = mesh_render_data_idx};
        cb.pushConstants(layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(RenderPushConstants), &pc);
        cb.drawIndexed(index_count, 1, index_offset, 0, 0);
    }
} // namespace ve
