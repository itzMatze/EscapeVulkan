#include "vk/Mesh.hpp"

namespace ve
{
    Mesh::Mesh(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<std::string>& texture_names, DescriptorSetHandler& dsh, const std::unordered_map<std::string, Image>& textures) : vertex_buffer(vmc, vertices, vk::BufferUsageFlagBits::eVertexBuffer, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, vcc), index_buffer(vmc, indices, vk::BufferUsageFlagBits::eIndexBuffer, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, vcc), dsh(dsh), texture_names(texture_names)
    {
        descriptor_set_start_idx = dsh.new_set();
        dsh.add_image_binding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, textures.find(/*texture_names[0]*/"ANY")->second);
    }

    void Mesh::self_destruct()
    {
        vertex_buffer.self_destruct();
        index_buffer.self_destruct();
    }

    void Mesh::draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, uint32_t current_frame)
    {
        std::vector<vk::DeviceSize> offsets(1, 0);

        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, dsh.get_sets()[descriptor_set_start_idx + current_frame], {});
        cb.bindVertexBuffers(0, vertex_buffer.get(), offsets);
        cb.bindIndexBuffer(index_buffer.get(), 0, vk::IndexType::eUint32);
        cb.drawIndexed(index_buffer.get_element_count(), 1, 0, 0, 0);
    }
}// namespace ve
