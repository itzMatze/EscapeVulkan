#include "vk/Mesh.hpp"

namespace ve
{
    Mesh::Mesh(int32_t material_idx, uint32_t idx_offset, uint32_t idx_count) : mat_idx(material_idx), index_offset(idx_offset), index_count(idx_count)
    {}

    void Mesh::draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, const std::vector<vk::DescriptorSet>& sets, DrawInfo& di)
    {
        PushConstants pc{.mat_idx = mat_idx, .light_count = di.light_count, .time = di.time, .normal_view = di.normal_view, .tex_view = di.tex_view};
        cb.pushConstants(layout, vk::ShaderStageFlagBits::eFragment, PushConstants::get_fragment_push_constant_offset(), PushConstants::get_fragment_push_constant_size(), pc.get_fragment_push_constant_pointer());
        cb.drawIndexed(index_count, 1, index_offset, 0, 0);
    }
} // namespace ve
