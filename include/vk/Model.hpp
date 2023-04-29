#pragma once

#include <string>
#include <vector>

#include "tiny_gltf.h"
#include "json.hpp"

#include "vk/Image.hpp"
#include "vk/Mesh.hpp"
#include "Storage.hpp"

namespace ve
{
    struct Model
    {
        Model() : meshes(static_cast<uint32_t>(ShaderFlavor::Size))
        {}

        std::vector<Mesh>& get_mesh_list(ShaderFlavor flavor)
        {
            return meshes.at(static_cast<uint32_t>(flavor));
        }

        void add_mesh(ShaderFlavor flavor, const Mesh& mesh)
        {
            meshes[static_cast<uint32_t>(flavor)].push_back(mesh);
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<uint32_t> texture_indices;
        std::vector<Material> materials;
        std::vector<std::vector<unsigned char>> texture_data;
        vk::Extent2D texture_dimensions;
    private:
        std::vector<std::vector<Mesh>> meshes;
    };

    namespace ModelLoader
    {
        Model load(const VulkanMainContext& vmc, Storage& storage, const std::string& path, uint32_t idx_count, uint32_t vertex_count, uint32_t material_count, uint32_t texture_count);
        Model load(const VulkanMainContext& vmc, Storage& storage, const nlohmann::json& model, uint32_t idx_count, uint32_t vertex_count, uint32_t material_count);
    };
} // namespace ve
