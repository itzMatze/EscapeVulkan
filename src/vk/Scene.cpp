#include "vk/Scene.hpp"

#include <string>
#include <regex>

#include "vk/DescriptorSetHandler.hpp"
#include "vk/common.hpp"

namespace ve
{
    Scene::Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc, DescriptorSetHandler& dsh, const std::string& path, const glm::mat4& transformation) : vmc(vmc), vcc(vcc), name(path.substr(path.find_last_of('/'), path.length())), dir(path.substr(0, path.find_last_of('/'))), transformation(transformation)
    {
        VE_LOG_CONSOLE(VE_INFO, "Loading scene \"" << name << "\"\n");
        textures.emplace("ANY", Image(vmc, vcc, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, "../assets/textures/white.png"));
        load_scene(path, dsh);
    }

    void Scene::self_destruct()
    {
        for (auto& mesh: meshes)
        {
            mesh.self_destruct();
        }
        meshes.clear();
        for (auto& texture: textures)
        {
            texture.second.self_destruct();
        }
        textures.clear();
    }

    void Scene::draw(uint32_t current_frame, const vk::PipelineLayout& layout, const glm::mat4& vp)
    {
        PushConstants pc{vp * transformation};
        vcc.graphics_cb[current_frame].pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &pc);
        for (auto& mesh: meshes)
        {
            mesh.draw(vcc.graphics_cb[current_frame], layout, current_frame);
        }
    }

    void Scene::load_scene(const std::string& path, DescriptorSetHandler& dsh)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            VE_THROW("Failed to load scene \"" << path << "\": " << importer.GetErrorString());
        }

        process_node(scene->mRootNode, scene, dsh);
    }

    void Scene::process_node(aiNode* node, const aiScene* scene, DescriptorSetHandler& dsh)
    {
        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(process_mesh(mesh, scene, dsh));
        }
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            process_node(node->mChildren[i], scene, dsh);
        }
    }

    Mesh Scene::process_mesh(aiMesh* mesh, const aiScene* scene, DescriptorSetHandler& dsh)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        // process vertices
        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            Vertex v;
            v.pos.x = mesh->mVertices[i].x;
            v.pos.y = mesh->mVertices[i].y;
            v.pos.z = mesh->mVertices[i].z;
            v.normal.x = mesh->mNormals[i].x;
            v.normal.y = mesh->mNormals[i].y;
            v.normal.z = mesh->mNormals[i].z;

            if (mesh->mColors[0])
            {
                v.color.r = mesh->mColors[0][i].r;
                v.color.g = mesh->mColors[0][i].g;
                v.color.b = mesh->mColors[0][i].b;
                //v.color.a = mesh->mColors[0][i].a;
            }
            else
            {
                aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

                aiColor3D color(0.f, 0.f, 0.f);
                material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
                v.color.r = color.r;
                v.color.g = color.g;
                v.color.b = color.b;

                //v.color = glm::vec3(0.0f, 0.0f, 0.0f);
            }
            if (mesh->mTextureCoords[0])
            {
                v.tex.x = mesh->mTextureCoords[0][i].x;
                v.tex.y = mesh->mTextureCoords[0][i].y;
            }
            else
            {
                v.tex = glm::vec2(-1.0f, -1.0f);
            }
            vertices.push_back(v);
        }
        // process indices
        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; ++j)
                indices.push_back(face.mIndices[j]);
        }
        // process material
        std::vector<std::string> texture_names;
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            std::vector<std::string> diffuse_names = load_textures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            texture_names.insert(texture_names.end(), diffuse_names.begin(), diffuse_names.end());
            //std::vector<Image> specular_textures = load_textures(material, aiTextureType_SPECULAR, "texture_specular");
            //textures.insert(textures.end(), specular_textures.begin(), specular_textures.end());
        }

        return Mesh(vmc, vcc, vertices, indices, texture_names, dsh, textures);
    }

    std::vector<std::string> Scene::load_textures(aiMaterial* mat, aiTextureType type, std::string typeName)
    {
        std::vector<std::string> filenames;
        for (uint32_t i = 0; i < mat->GetTextureCount(type); ++i)
        {
            aiString ai_name;
            mat->GetTexture(type, i, &ai_name);
            filenames.push_back(ai_name.C_Str());
            if (!textures.contains(filenames.back()))
            {
                std::string s = filenames.back();
                std::replace(s.begin(), s.end(), '\\', '/');
                Image texture(vmc, vcc, {uint32_t(vmc.queues_family_indices.graphics)}, dir + "/" + s);
                textures.emplace(filenames.back(), texture);
            }
            return filenames;// we currently only can handle one texture
        }
        return {};
    }
}// namespace ve
