#include "vk/Mesh.hpp"

namespace ve
{
    Mesh::Mesh(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<std::string>& texture_names) : vertex_buffer(vmc, vertices, vk::BufferUsageFlagBits::eVertexBuffer, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, vcc), index_buffer(vmc, indices, vk::BufferUsageFlagBits::eIndexBuffer, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, vcc), texture_names(texture_names)
    {}

    void Mesh::self_destruct()
    {
        vertex_buffer.self_destruct();
        index_buffer.self_destruct();
    }

    void Mesh::add_set_bindings(DescriptorSetHandler& dsh, const std::unordered_map<std::string, Image>& textures)
    {
        descriptor_set_indices.push_back(dsh.new_set());
        dsh.add_binding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, textures.find(/*texture_names[0]*/ "ANY")->second);
    }

    void Mesh::draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, const std::vector<vk::DescriptorSet>& sets, uint32_t current_frame)
    {
        std::vector<vk::DeviceSize> offsets(1, 0);

        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, sets[descriptor_set_indices[current_frame]], {});
        cb.bindVertexBuffers(0, vertex_buffer.get(), offsets);
        cb.bindIndexBuffer(index_buffer.get(), 0, vk::IndexType::eUint32);
        cb.drawIndexed(index_buffer.get_element_count(), 1, 0, 0, 0);
    }
}// namespace ve
