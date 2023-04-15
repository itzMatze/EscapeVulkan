#include "vk/Mesh.hpp"

namespace ve
{
    Mesh::Mesh(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const Material& material, uint32_t idx_offset, uint32_t idx_count) : mat(material), index_offset(idx_offset), index_count(idx_count)
    {}

    void Mesh::self_destruct()
    {}

    void Mesh::add_set_bindings(DescriptorSetHandler& dsh)
    {
        descriptor_set_indices.push_back(dsh.new_set());
        if (mat.base_texture != nullptr) dsh.add_descriptor(1, *(mat.base_texture));
    }

    void Mesh::draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, const std::vector<vk::DescriptorSet>& sets, uint32_t current_frame)
    {
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, sets[descriptor_set_indices[current_frame]], {});
        cb.drawIndexed(index_count, 1, index_offset, 0, 0);
    }
}// namespace ve
