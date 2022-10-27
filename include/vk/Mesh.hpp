#pragma once

#include "vk/DescriptorSetHandler.hpp"
#include "vk/Image.hpp"
#include "vk/common.hpp"

namespace ve
{
    struct Material {
        float metallic = 1.0f;
        float roughness = 1.0f;
        glm::vec4 base_color = glm::vec4(1.0f);
        glm::vec4 emission = glm::vec4(1.0f);
        Image* base_texture;
        Image* metallic_roughness_texture;
        Image* normal_texture;
        Image* occlusion_texture;
        Image* emissive_texture;
    };

    class Mesh
    {
    public:
        Mesh(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const Material* material);
        void self_destruct();
        void add_set_bindings(DescriptorSetHandler& dsh);
        void draw(vk::CommandBuffer& cb, const vk::PipelineLayout layout, const std::vector<vk::DescriptorSet>& sets, uint32_t current_frame);

    private:
        Buffer vertex_buffer;
        Buffer index_buffer;
        std::vector<uint32_t> descriptor_set_indices;
        const Material* mat;
    };
}// namespace ve
