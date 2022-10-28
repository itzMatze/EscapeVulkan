#pragma once

#include <string>
#include <vector>

#include "tiny_gltf.h"

#include "vk/Image.hpp"
#include "vk/Mesh.hpp"

namespace ve
{
    class Scene
    {
    public:
        Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc, const std::string& path);
        Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const Material* material);
        void self_destruct();
        void add_set_bindings(DescriptorSetHandler& dsh);
        void draw(uint32_t current_frame, const vk::PipelineLayout& layout, const std::vector<vk::DescriptorSet>& sets, const glm::mat4& vp);
        void translate(const glm::vec3& trans);
        void scale(const glm::vec3& scale);
        void rotate(float degree, const glm::vec3& axis);

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        std::vector<Mesh> meshes;
        std::vector<Image> textures;
        std::vector<Material> materials;
        std::string name;
        std::string dir;
        glm::mat4 transformation;

        void load_scene(const std::string& path);
        void process_node(const tinygltf::Node& node, const tinygltf::Model& model, const glm::mat4 trans);
        void process_mesh(const tinygltf::Mesh& mesh, const tinygltf::Model& model, const glm::mat4 matrix);
    };
}// namespace ve
