#include "vk/Model.hpp"

#include <string>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#include <glm/gtc/type_ptr.hpp>

#include "vk/DescriptorSetHandler.hpp"
#include "vk/common.hpp"

namespace ve
{
    namespace ModelLoader
    {
        // all indices need to be offset by the amount of data that will be stored in front
        uint32_t total_index_count;
        uint32_t total_vertex_count;
        uint32_t total_material_count;
        uint32_t total_texture_count;
        // materials and textures are loaded when they are needed which requires to know if a texture or material is already loaded (-1 = not loaded)
        std::vector<int32_t> texture_indices;
        std::vector<int32_t> material_indices;

        Material& load_material(const VulkanMainContext& vmc, Storage& storage, int mat_idx, const tinygltf::Model& model, Model& model_data)
        {
            if (mat_idx < 0) VE_THROW("Trying to load material_idx < 0!");
            const tinygltf::Material& mat = model.materials[mat_idx];

            auto get_texture = [&](const std::string& name, uint32_t base_mip_level) -> int32_t {
                if (mat.values.find(name) == mat.values.end()) return -1;
                // check if texture is already loaded and if not load it
                int texture_idx = mat.values.at(name).TextureIndex();
                if (texture_indices[texture_idx] > -1) return texture_indices[texture_idx];
                const tinygltf::Texture& tex = model.textures[texture_idx];
                texture_indices[texture_idx] = storage.add_image(model.images[tex.source].image.data(), model.images[tex.source].width, model.images[tex.source].height, true, base_mip_level, std::vector<uint32_t>{vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics}, vk::ImageUsageFlagBits::eSampled);
                std::cout << model.images[tex.source].width << ";" << model.images[tex.source].height << std::endl;
                return texture_indices[texture_idx];
            };

            auto get_array_layer_texture = [&](const std::string& name, std::vector<std::vector<unsigned char>>& images, vk::Extent2D& dimensions) -> int32_t {
                if (mat.values.find(name) == mat.values.end()) return -1;
                // check if texture is already loaded and if not load it
                int texture_idx = mat.values.at(name).TextureIndex();
                if (texture_indices[texture_idx] > -1) return texture_indices[texture_idx];
                const tinygltf::Texture& tex = model.textures[texture_idx];
                texture_indices[texture_idx] = images.size() + total_texture_count;
                images.push_back(model.images[tex.source].image);
                dimensions.width = model.images[tex.source].width;
                dimensions.height = model.images[tex.source].height;
                return texture_indices[texture_idx];
            };

            Material material{};
            material.base_texture = get_array_layer_texture("baseColorTexture", model_data.texture_data, model_data.texture_dimensions);
            //material.metallic_roughness_texture = get_texture("metallicRoughnessTexture", 1);
            //material.normal_texture = get_texture("normalTexture", 1);
            //material.emissive_texture = get_texture("emissiveTexture", 1);
            //material.occlusion_texture = get_texture("occlusionTexture", 1);
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
            model_data.materials.push_back(material);
            material_indices[mat_idx] = model_data.materials.size() + total_material_count - 1;
            return model_data.materials.back();
        }

        void process_mesh(const VulkanMainContext& vmc, Storage& storage, const tinygltf::Mesh& mesh, const tinygltf::Model& model, const glm::mat4 matrix, Model& model_data)
        {
            ShaderFlavor flavor;
            for (const tinygltf::Primitive& primitive : mesh.primitives)
            {
                uint32_t idx_count = model_data.indices.size();
                uint32_t vertex_count = model_data.vertices.size();
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

                    // load access information
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

                    // access data and load vertices
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
                        else
                        {
                            vertex.color = glm::vec4(1.0f);
                        }
                        vertex.tex = tex_buffer ? glm::make_vec2(&tex_buffer[i * tex_stride]) : glm::vec2(-1.0f);
                        model_data.vertices.push_back(vertex);
                    }
                }
                // indices
                const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
                const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
                const void* raw_data = &(model.buffers[buffer_view.buffer].data[accessor.byteOffset + buffer_view.byteOffset]);

                auto add_indices([&](const auto* buf) -> void {
                    for (size_t i = 0; i < accessor.count; ++i)
                    {
                        model_data.indices.push_back(buf[i] + total_vertex_count + vertex_count);
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
                if (primitive.material > -1)
                {
                    Material& mat = load_material(vmc, storage, primitive.material, model, model_data);
                    if (mat.base_texture > -1)
                    {
                        flavor = ShaderFlavor::Default;
                    }
                    else if (mat.emission.r == 1.0f || mat.emission.g == 1.0f || mat.emission.b == 1.0f)
                    {
                        flavor = ShaderFlavor::Emissive;
                    }
                    else
                    {
                        flavor = ShaderFlavor::Basic;
                    }
                    model_data.add_mesh(flavor, Mesh(material_indices[primitive.material], total_index_count + idx_count, model_data.indices.size() - idx_count, mesh.name));
                }
                else
                {
                    model_data.add_mesh(ShaderFlavor::Basic, Mesh(-1, total_index_count + idx_count, model_data.indices.size() - idx_count, mesh.name));
                }
            }
        }

        void process_node(const VulkanMainContext& vmc, Storage& storage, const tinygltf::Node& node, const tinygltf::Model& model, const glm::mat4 trans, Model& model_data)
        {
            glm::vec3 translation = (node.translation.size() == 3) ? glm::make_vec3(node.translation.data()) : glm::dvec3(0.0f);
            glm::quat q = (node.rotation.size() == 4) ? glm::make_quat(node.rotation.data()) : glm::qua<double>();
            glm::vec3 scale = (node.scale.size() == 3) ? glm::make_vec3(node.scale.data()) : glm::dvec3(1.0f);
            glm::mat4 matrix = (node.matrix.size() == 16) ? glm::make_mat4x4(node.matrix.data()) : glm::dmat4(1.0f);
            matrix = trans * glm::translate(glm::mat4(1.0f), translation) * glm::mat4(q) * glm::scale(glm::mat4(1.0f), scale) * matrix;
            for (auto& child_idx : node.children)
            {
                process_node(vmc, storage, model.nodes[child_idx], model, matrix, model_data);
            }
            if (node.mesh > -1) (process_mesh(vmc, storage, model.meshes[node.mesh], model, matrix, model_data));
            if (node.extensions.contains("KHR_lights_punctual"))
            {
                const auto& lights = node.extensions.at("KHR_lights_punctual");
                int32_t light_idx = lights.Get("light").GetNumberAsInt();
                const auto& light = model.lights[light_idx];
                
                Light l;
                l.dir = glm::vec3(glm::normalize(matrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
                glm::vec4 pos = matrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                pos.x /= pos.w;
                pos.y /= pos.w;
                pos.z /= pos.w;
                l.pos = glm::vec3(pos);

                VE_ASSERT(light.color.size() > 2, "Light with less than 3 color components found!");
                l.color.r = light.color[0];
                l.color.g = light.color[1];
                l.color.b = light.color[2];

                l.intensity = light.intensity * 20.0;

                l.innerConeAngle = std::cos(light.spot.innerConeAngle);
                l.outerConeAngle = std::cos(light.spot.outerConeAngle);
                model_data.lights.push_back(l);
            }
        }

        Model load(const VulkanMainContext& vmc, Storage& storage, const std::string& path, uint32_t idx_count, uint32_t vertex_count, uint32_t material_count, uint32_t texture_count)
        {
            total_index_count = idx_count;
            total_vertex_count = vertex_count;
            total_material_count = material_count;
            Model model_data{};
            spdlog::info("Loading glb: \"{}\"", path);
            tinygltf::TinyGLTF loader;
            tinygltf::Model model;
            std::string err;
            std::string warn;
            if (!loader.LoadBinaryFromFile(&model, &err, &warn, path)) VE_THROW("Failed to load glb: \"{}\"", path);
            if (!warn.empty()) spdlog::warn(warn);
            if (!err.empty()) VE_THROW(err);

            texture_indices.resize(model.textures.size(), -1);
            material_indices.resize(model.materials.size(), -1);

            const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
            // traverse scene nodes
            for (auto& node_idx : scene.nodes)
            {
                process_node(vmc, storage, model.nodes[node_idx], model, glm::mat4(1.0f), model_data);
            }
            for (const auto& i : texture_indices)
            {
                if (i > -1) model_data.texture_indices.push_back(i);
            }
            texture_indices.clear();
            material_indices.clear();
            return model_data;
        }

        Model load(const VulkanMainContext& vmc, Storage& storage, const nlohmann::json& model, uint32_t idx_count, uint32_t vertex_count, uint32_t material_count)
        {
            total_index_count = idx_count;
            total_vertex_count = vertex_count;
            total_material_count = material_count;
            Model model_data{};
            // load custom directly in json defined models
            for (auto& v : model.at("vertices"))
            {
                Vertex vertex;
                vertex.pos = glm::vec3(v.at("pos")[0], v.at("pos")[1], v.at("pos")[2]);
                vertex.normal = glm::vec3(v.at("normal")[0], v.at("normal")[1], v.at("normal")[2]);
                vertex.color = glm::vec4(v.at("color")[0], v.at("color")[1], v.at("color")[2], v.at("color")[3]);
                vertex.tex = glm::vec2(v.at("tex")[0], v.at("tex")[1]);
                model_data.vertices.push_back(vertex);
            }
            for (auto& i : model.at("indices"))
            {
                model_data.indices.push_back(uint32_t(i) + total_vertex_count);
            }
            Material m;
            if (model.contains("base_texture"))
            {
                texture_indices.emplace_back(storage.add_image(std::string("../assets/textures/") + std::string(model.value("base_texture", "")), true, 0, std::vector<uint32_t>{vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics}, vk::ImageUsageFlagBits::eSampled));
                m.base_texture = texture_indices.back();
            }
            model_data.materials.push_back(m);
            material_indices.push_back(model_data.materials.size() + total_material_count - 1);
            if (model.value("ShaderFlavor", "") == "Basic")
            {
                model_data.add_mesh(ShaderFlavor::Basic, Mesh(material_indices.back(), total_index_count, model_data.indices.size(), "custom_model"));
            }
            else if (model.value("ShaderFlavor", "") == "Default")
            {
                model_data.add_mesh(ShaderFlavor::Default, Mesh(material_indices.back(), total_index_count, model_data.indices.size(), "custom_model"));
            }
            else
            {
                VE_THROW("Unknown ShaderFlavor");
            }
            for (const auto& i : texture_indices)
            {
                if (i > -1) model_data.texture_indices.push_back(i);
            }
            texture_indices.clear();
            material_indices.clear();
            return model_data;
        }

    } // namespace ModelLoader
} // namespace ve
