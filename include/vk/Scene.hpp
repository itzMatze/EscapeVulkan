#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <string>
#include <vector>

#include "vk/Image.hpp"
#include "vk/Mesh.hpp"

namespace ve
{
    class Scene
    {
    public:
        Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc, DescriptorSetHandler& dsh, const std::string& path, const glm::mat4& transformation);
        void self_destruct();
        void draw(uint32_t current_frame, const vk::PipelineLayout& layout, const glm::mat4& vp);

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        std::vector<Mesh> meshes;
        std::unordered_map<std::string, Image> textures;
        std::string name;
        std::string dir;
        glm::mat4 transformation;

        void load_scene(const std::string& path, DescriptorSetHandler& dsh);
        void process_node(aiNode* node, const aiScene* scene, DescriptorSetHandler& dsh);
        Mesh process_mesh(aiMesh* mesh, const aiScene* scene, DescriptorSetHandler& dsh);
        std::vector<std::string> load_textures(aiMaterial* mat, aiTextureType type, std::string typeName);
    };
}// namespace ve
