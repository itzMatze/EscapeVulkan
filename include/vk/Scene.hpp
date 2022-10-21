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
        Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc, const std::string& path, const glm::mat4& transformation);
        void self_destruct();
        void add_set_bindings(DescriptorSetHandler& dsh);
        void draw(uint32_t current_frame, const vk::PipelineLayout& layout, const std::vector<vk::DescriptorSet>& sets, const glm::mat4& vp);

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        std::vector<Mesh> meshes;
        std::unordered_map<std::string, Image> textures;
        std::string name;
        std::string dir;
        glm::mat4 transformation;

        void load_scene(const std::string& path);
        void process_node(aiNode* node, const aiScene* scene);
        Mesh process_mesh(aiMesh* mesh, const aiScene* scene);
        std::vector<std::string> load_textures(aiMaterial* mat, aiTextureType type, std::string typeName);
    };
}// namespace ve
