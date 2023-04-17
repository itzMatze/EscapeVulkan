#include "vk/Model.hpp"

#include <string>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "vk/DescriptorSetHandler.hpp"
#include "vk/common.hpp"

namespace ve
{
    Model::Model(const VulkanMainContext& vmc, VulkanCommandContext& vcc, VulkanStorageContext& vsc, const std::string name, const std::string& path) : vmc(vmc), vcc(vcc), vsc(vsc), name(name), transformation(glm::mat4(1.0f))
    {
        spdlog::info("Loading glb: \"{}\"", path);
        load_model(path);
    }

    Model::Model(const VulkanMainContext& vmc, VulkanCommandContext& vcc, VulkanStorageContext& vsc, const nlohmann::json& model) : vmc(vmc), vcc(vcc), vsc(vsc), transformation(glm::mat4(1.0f))
    {
        // load custom directly in json defined models
        name = model.value("name", "");
        for (auto& v: model.at("vertices"))
        {
            Vertex vertex;
            vertex.pos = glm::vec3(v.at("pos")[0], v.at("pos")[1], v.at("pos")[2]);
            vertex.normal = glm::vec3(v.at("normal")[0], v.at("normal")[1], v.at("normal")[2]);
            vertex.color = glm::vec4(v.at("color")[0], v.at("color")[1], v.at("color")[2], v.at("color")[3]);
            vertex.tex = glm::vec2(v.at("tex")[0], v.at("tex")[1]);
            vertices.push_back(vertex);
        }
        for (auto& i: model.at("indices"))
        {
            indices.push_back(i);
        }
        Material m;
        if (model.contains("base_texture"))
        {
            textures.emplace_back(vsc.add_image(vmc, vcc, std::string("../assets/textures/") + std::string(model.value("base_texture", "")), vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));
            m.base_texture = textures.back().value();
        }
        materials.push_back(m);
        vertex_buffer = vsc.add_named_buffer(std::string(name + std::string("_v")), vmc, vertices, vk::BufferUsageFlagBits::eVertexBuffer, true, vcc, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        index_buffer = vsc.add_named_buffer(std::string(name + std::string("_i")), vmc, indices, vk::BufferUsageFlagBits::eIndexBuffer, true, vcc, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        meshes.emplace_back(Mesh(vmc, vcc, materials.back().value(), 0, indices.size()));
        vertices.clear();
        indices.clear();
    }

    void Model::add_set_bindings(DescriptorSetHandler& dsh)
    {
        for (auto& mesh: meshes)
        {
            mesh.add_set_bindings(dsh, vsc);
        }
    }

    void Model::self_destruct()
    {
        vsc.destroy_buffer(vertex_buffer);
        vsc.destroy_buffer(index_buffer);
        for (auto& mesh: meshes)
        {
            mesh.self_destruct();
        }
        meshes.clear();
        for (auto& texture : textures)
        {
            if (texture.has_value()) vsc.destroy_image(texture.value());
        }
        textures.clear();
    }

    void Model::draw(uint32_t current_frame, const vk::PipelineLayout& layout, const std::vector<vk::DescriptorSet>& sets, const glm::mat4& vp)
    {
        PushConstants pc{vp * transformation};
        vcc.graphics_cb[current_frame].pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &pc);
        vcc.graphics_cb[current_frame].bindVertexBuffers(0, vsc.get_buffer(vertex_buffer).get(), {0});
        vcc.graphics_cb[current_frame].bindIndexBuffer(vsc.get_buffer(index_buffer).get(), 0, vk::IndexType::eUint32);
        for (auto& mesh: meshes)
        {
            mesh.draw(vcc.graphics_cb[current_frame], layout, sets, current_frame);
        }
    }

    void Model::translate(const glm::vec3& trans)
    {
        transformation = glm::translate(trans) * transformation;
    }

    void Model::scale(const glm::vec3& scale)
    {
        transformation = glm::scale(scale) * transformation;
    }

    void Model::rotate(float degree, const glm::vec3& axis)
    {
        glm::vec3 translation = transformation[3];
        transformation[3] = glm::vec4(0.0f, 0.0f, 0.0f, transformation[3].w);
        transformation = glm::rotate(glm::radians(degree), axis) * transformation;
        translate(translation);
    }

    void Model::load_model(const std::string& path)
    {
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string err;
        std::string warn;
        if (!loader.LoadBinaryFromFile(&model, &err, &warn, path)) VE_THROW("Failed to load glb: \"{}\"", path);
        if (!warn.empty()) spdlog::warn(warn);
        if (!err.empty()) VE_THROW(err);

        textures.resize(model.textures.size());
        materials.resize(model.materials.size() + 1);
        Material default_mat;
        default_mat.base_texture = std::nullopt;
        default_mat.metallic_roughness_texture = std::nullopt;
        default_mat.normal_texture = std::nullopt;
        default_mat.emissive_texture = std::nullopt;
        default_mat.occlusion_texture = std::nullopt;
        materials.back().emplace(default_mat);

        const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
        // traverse scene nodes
        for (auto& node_idx: scene.nodes)
        {
            process_node(model.nodes[node_idx], model, glm::mat4(1.0f));
        }
        vertex_buffer = vsc.add_named_buffer(std::string(name + std::string("_v")), vmc, vertices, vk::BufferUsageFlagBits::eVertexBuffer, true, vcc, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        index_buffer = vsc.add_named_buffer(std::string(name + std::string("_i")), vmc, indices, vk::BufferUsageFlagBits::eIndexBuffer, true, vcc, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        // delete vertices and indices on host
        indices.clear();
        vertices.clear();
    }

    Material& Model::load_material(int mat_idx, const tinygltf::Model& model)
    {
        if (mat_idx < 0) return materials.back().value();
        const tinygltf::Material& mat = model.materials[mat_idx];

        auto get_texture = [&](const std::string& name, uint32_t base_mip_level) -> std::optional<uint32_t> {
            if (mat.values.find(name) == mat.values.end()) return std::nullopt;
            int texture_idx = mat.values.at(name).TextureIndex();
            if (textures[texture_idx].has_value()) return textures[texture_idx];
            const tinygltf::Texture& tex = model.textures[texture_idx];
            Image base_image(vmc, vcc, model.images[tex.source].image.data(), model.images[tex.source].width, model.images[tex.source].height, true, vmc.queue_family_indices.transfer);
            textures[texture_idx].emplace(vsc.add_image(vmc, vcc, base_image, base_mip_level, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));
            base_image.self_destruct();
            return textures[texture_idx];
        };

        Material material{};
        material.base_texture = get_texture("baseColorTexture", 1);
        material.metallic_roughness_texture = get_texture("metallicRoughnessTexture", 1);
        material.normal_texture = get_texture("normalTexture", 1);
        material.emissive_texture = get_texture("emissiveTexture", 1);
        material.occlusion_texture = get_texture("occlusionTexture", 1);
        if (mat.values.find("baseColorFactor") != mat.values.end())
        {
            material.base_color = glm::make_vec4(mat.values.at("baseColorFactor").ColorFactor().data());
        }
        if (mat.values.find("metallicFactor") != mat.values.end())
        {
            material.metallic = static_cast<float>(mat.values.at("metallicFactor").Factor());
        }
        if (mat.values.find("roughnessFactor") != mat.values.end())
        {
            material.roughness = static_cast<float>(mat.values.at("roughnessFactor").Factor());
        }
        if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end())
        {
            material.emission = glm::vec4(glm::make_vec3(mat.additionalValues.at("emissiveFactor").ColorFactor().data()), 1.0);
        }
        materials[mat_idx].emplace(material);
        return materials[mat_idx].value();
    }

    void Model::process_node(const tinygltf::Node& node, const tinygltf::Model& model, const glm::mat4 trans)
    {
        glm::vec3 translation = (node.translation.size() == 3) ? glm::make_vec3(node.translation.data()) : glm::dvec3(0.0f);
        glm::quat q = (node.rotation.size() == 4) ? glm::make_quat(node.rotation.data()) : glm::qua<double>();
        glm::vec3 scale = (node.scale.size() == 3) ? glm::make_vec3(node.scale.data()) : glm::dvec3(1.0f);
        glm::mat4 matrix = (node.matrix.size() == 16) ? glm::make_mat4x4(node.matrix.data()) : glm::dmat4(1.0f);
        matrix = trans * glm::translate(glm::mat4(1.0f), translation) * glm::mat4(q) * glm::scale(glm::mat4(1.0f), scale) * matrix;
        for (auto& child_idx: node.children)
        {
            process_node(model.nodes[child_idx], model, matrix);
        }
        if (node.mesh > -1) (process_mesh(model.meshes[node.mesh], model, matrix));
    }

    void Model::process_mesh(const tinygltf::Mesh& mesh, const tinygltf::Model& model, const glm::mat4 matrix)
    {
        for (const tinygltf::Primitive& primitive: mesh.primitives)
        {
            uint32_t idx_count = indices.size();
            Material& mat = load_material(primitive.material, model);
            // vertices
            {
                const float* pos_buffer = nullptr;
                const float* normal_buffer = nullptr;
                const float* tex_buffer = nullptr;
                const float* color_buffer = nullptr;

                int pos_stride;
                int normal_stride;
                int tex_stride;
                int color_stride;

                const tinygltf::Accessor& pos_accessor = model.accessors[primitive.attributes.find("POSITION")->second];
                const tinygltf::BufferView& pos_view = model.bufferViews[pos_accessor.bufferView];
                pos_buffer = reinterpret_cast<const float*>(&(model.buffers[pos_view.buffer].data[pos_accessor.byteOffset + pos_view.byteOffset]));
                pos_stride = pos_accessor.ByteStride(pos_view) ? (pos_accessor.ByteStride(pos_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
                {
                    const tinygltf::Accessor& normal_accessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                    const tinygltf::BufferView& normal_view = model.bufferViews[normal_accessor.bufferView];
                    normal_buffer = reinterpret_cast<const float*>(&(model.buffers[normal_view.buffer].data[normal_accessor.byteOffset + normal_view.byteOffset]));
                    normal_stride = normal_accessor.ByteStride(normal_view) ? (normal_accessor.ByteStride(normal_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                }
                if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                {
                    const tinygltf::Accessor& tex_accessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                    const tinygltf::BufferView& tex_view = model.bufferViews[tex_accessor.bufferView];
                    tex_buffer = reinterpret_cast<const float*>(&(model.buffers[tex_view.buffer].data[tex_accessor.byteOffset + tex_view.byteOffset]));
                    tex_stride = tex_accessor.ByteStride(tex_view) ? (tex_accessor.ByteStride(tex_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
                }
                if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
                {
                    const tinygltf::Accessor& color_accessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
                    const tinygltf::BufferView& color_view = model.bufferViews[color_accessor.bufferView];
                    color_buffer = reinterpret_cast<const float*>(&(model.buffers[color_view.buffer].data[color_accessor.byteOffset + color_view.byteOffset]));
                    color_stride = color_accessor.ByteStride(color_view) ? (color_accessor.ByteStride(color_view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
                }

                for (size_t i = 0; i < pos_accessor.count; ++i)
                {
                    Vertex vertex;
                    glm::vec4 tmp_pos = matrix * glm::vec4(glm::make_vec3(&pos_buffer[i * pos_stride]), 1.0f);
                    vertex.pos = glm::vec3(tmp_pos.x / tmp_pos.w, tmp_pos.y / tmp_pos.w, tmp_pos.z / tmp_pos.w);
                    VE_ASSERT(normal_buffer, "No normals in this model!");
                    vertex.normal = glm::normalize(glm::make_vec3(&normal_buffer[i * normal_stride]));
                    if (color_buffer)
                    {
                        vertex.color = glm::make_vec4(&color_buffer[i * color_stride]);
                    }
                    else if (primitive.material > -1 && mat.base_color.length() > 0.0f)
                    {
                        vertex.color = glm::vec4(mat.base_color);
                    }
                    else
                    {
                        vertex.color = glm::vec4(1.0f);
                    }
                    vertex.tex = tex_buffer ? glm::make_vec2(&tex_buffer[i * tex_stride]) : glm::vec2(-1.0f);
                    vertices.push_back(vertex);
                }
            }
            // indices
            const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
            const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
            const void* raw_data = &(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]);

            auto add_indices([&](const auto* buf) -> void {
                for (size_t i = 0; i < accessor.count; ++i)
                {
                    indices.push_back(buf[i] + vertex_count);
                }
            });
            switch (accessor.componentType)
            {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                    add_indices(static_cast<const uint32_t*>(raw_data));
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    add_indices(static_cast<const uint16_t*>(raw_data));
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                    add_indices(static_cast<const uint8_t*>(raw_data));
                    break;
                default:
                    VE_THROW("Index component type \"{}\" not supported!", accessor.componentType);
            }
            vertex_count = vertices.size();
            meshes.emplace_back(Mesh(vmc, vcc, mat, idx_count, indices.size() - idx_count));
        }
    }
} // namespace ve
