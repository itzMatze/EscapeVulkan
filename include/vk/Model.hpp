#pragma once

#include <string>
#include <vector>

#include "tiny_gltf.h"

#include "vk/Image.hpp"
#include "vk/Mesh.hpp"
#include "json.hpp"

namespace ve
{
    class Model
    {
    public:
        Model(const VulkanMainContext& vmc, VulkanCommandContext& vcc, const std::string& path);
        Model(const VulkanMainContext& vmc, VulkanCommandContext& vcc, const nlohmann::json& model);
        void self_destruct();
        void add_set_bindings(DescriptorSetHandler& dsh);
        void draw(uint32_t current_frame, const vk::PipelineLayout& layout, const std::vector<vk::DescriptorSet>& sets, const glm::mat4& vp);
        void translate(const glm::vec3& trans);
        void scale(const glm::vec3& scale);
        void rotate(float degree, const glm::vec3& axis);

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        std::vector<Vertex> vertices;
        uint32_t vertex_count = 0;
        std::vector<uint32_t> indices;
        Buffer vertex_buffer;
        Buffer index_buffer;
        std::vector<Mesh> meshes;
        std::vector<std::optional<Image>> textures;
        std::vector<std::optional<Material>> materials;
        std::string name;
        glm::mat4 transformation;

        void load_model(const std::string& path);
        Material& load_material(int mat_idx, const tinygltf::Model& model);
        void process_node(const tinygltf::Node& node, const tinygltf::Model& model, const glm::mat4 trans);
        void process_mesh(const tinygltf::Mesh& mesh, const tinygltf::Model& model, const glm::mat4 matrix);
    };
}// namespace ve
