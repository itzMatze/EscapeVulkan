#include "vk/Mesh.hpp"

namespace ve
{
    Mesh::Mesh(const Material& material, uint32_t idx_offset, uint32_t idx_count) : mat(material), index_offset(idx_offset), index_count(idx_count)
    {}

    void Mesh::add_set_bindings(DescriptorSetHandler& dsh, VulkanStorageContext& vsc)
    {
        // create new descriptor set and add descriptor(s) needed for rendering
        descriptor_set_indices.push_back(dsh.new_set());
        if (mat.base_texture.has_value()) dsh.add_descriptor(1, vsc.get_image(mat.base_texture.value()));
    }

    void Mesh::draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, const std::vector<vk::DescriptorSet>& sets, uint32_t current_frame)
    {
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, sets[descriptor_set_indices[current_frame]], {});
        cb.drawIndexed(index_count, 1, index_offset, 0, 0);
    }
} // namespace ve
