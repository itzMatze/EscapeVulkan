#include "vk/Scene.hpp"

#include <fstream>
#include <glm/gtx/transform.hpp>

#include "json.hpp"

namespace ve
{
    Scene::Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc, VulkanStorageContext& vsc) : vmc(vmc), vcc(vcc), vsc(vsc)
    {}

    void Scene::construct(const RenderPass& render_pass)
    {
        vk::SpecializationMapEntry uniform_buffer_size_entry(0, 0, sizeof(uint32_t));
        uint32_t uniform_buffer_size = model_render_data.size();
        vk::SpecializationInfo spec_info(1, &uniform_buffer_size_entry, sizeof(uint32_t), &uniform_buffer_size);
        std::vector<ShaderInfo> default_info;
        default_info.push_back(ShaderInfo{"default.vert", vk::ShaderStageFlagBits::eVertex, spec_info});
        default_info.push_back(ShaderInfo{"default.frag", vk::ShaderStageFlagBits::eFragment});
        ros.at(ShaderFlavor::Default).construct(render_pass, default_info);

        std::vector<ShaderInfo> basic_info;
        basic_info.push_back(ShaderInfo{"default.vert", vk::ShaderStageFlagBits::eVertex, spec_info});
        basic_info.push_back(ShaderInfo{"basic.frag", vk::ShaderStageFlagBits::eFragment});
        ros.at(ShaderFlavor::Basic).construct(render_pass, basic_info);

        std::vector<ShaderInfo> emissive_info;
        emissive_info.push_back(ShaderInfo{"default.vert", vk::ShaderStageFlagBits::eVertex, spec_info});
        emissive_info.push_back(ShaderInfo{"emissive.frag", vk::ShaderStageFlagBits::eFragment});
        ros.at(ShaderFlavor::Emissive).construct(render_pass, emissive_info);
    }

    void Scene::self_destruct()
    {
        vsc.destroy_buffer(vertex_buffer);
        vsc.destroy_buffer(index_buffer);
        if (material_buffer > -1) vsc.destroy_buffer(material_buffer);
        material_buffer = -1;
        for (auto& buffer : model_render_data_buffers) vsc.destroy_buffer(buffer);
        model_render_data_buffers.clear();
        model_render_data.clear();
        if (texture_image > -1) vsc.destroy_image(texture_image);
        texture_image = -1;
        for (auto& ro : ros) ro.second.self_destruct();
        ros.clear(); 
        model_handles.clear();
        model_transformations.clear();
        loaded = false;
    }

    void Scene::load(const std::string& path)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        vk::Extent2D texture_dimensions;

        auto add_model = [&](Model& model, const std::string& name) -> void
        {
            vertices.insert(vertices.end(), model.vertices.begin(), model.vertices.end());
            indices.insert(indices.end(), model.indices.begin(), model.indices.end());
            materials.insert(materials.end(), model.materials.begin(), model.materials.end());
            texture_data.insert(texture_data.begin(), model.texture_data.begin(), model.texture_data.end());
            if (!model.texture_data.empty())
            {
                VE_ASSERT(texture_dimensions == model.texture_dimensions || texture_dimensions == 0, "Textures have different dimensions!");
                texture_dimensions = model.texture_dimensions;
            }
            model_transformations.push_back(glm::mat4(1.0f));
            model_handles.emplace(name, model_transformations.size() - 1);
            for (auto& ro : ros)
            {
                ro.second.add_model_meshes(model.get_mesh_list(ro.first));
            }
        };

        ros.emplace(ShaderFlavor::Default, vmc);
        ros.emplace(ShaderFlavor::Basic, vmc);
        ros.emplace(ShaderFlavor::Emissive, vmc);

        ros.at(ShaderFlavor::Default).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        ros.at(ShaderFlavor::Default).dsh.add_binding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Default).dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);

        ros.at(ShaderFlavor::Basic).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

        ros.at(ShaderFlavor::Emissive).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        ros.at(ShaderFlavor::Emissive).dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        // load scene from custom json file
        using json = nlohmann::json;
        std::ifstream file(path);
        json data = json::parse(file);
        if (data.contains("model_files"))
        {
            // load referenced model files
            for (const auto& d : data.at("model_files"))
            {
                const std::string name = d.value("name", "");
                Model model = ModelLoader::load(vmc, vsc, std::string("../assets/models/") + std::string(d.value("file", "")), indices.size(), vertices.size(), materials.size(), texture_data.size());
                add_model(model, name);

                // apply transformations to model
                if (d.contains("scale"))
                {
                    glm::vec3 scaling(d.at("scale")[0], d.at("scale")[1], d.at("scale")[2]);
                    scale(name, scaling);
                }
                if (d.contains("translation"))
                {
                    glm::vec3 translation(d.at("translation")[0], d.at("translation")[1], d.at("translation")[2]);
                    translate(name, translation);
                }
                if (d.contains("rotation"))
                {
                    glm::vec3 rotation(d.at("rotation")[1], d.at("rotation")[2], d.at("rotation")[3]);
                    rotate(name, d.at("rotation")[0], rotation);
                }
            }
        }
        // load custom models (vertices and indices directly contained in json file)
        if (data.contains("custom_models"))
        {
            for (const auto& d : data["custom_models"])
            {
                std::string name = d.value("name", "");
                Model model = ModelLoader::load(vmc, vsc, d, indices.size(), vertices.size(), materials.size());
                add_model(model, name);
            }
        }
        vertex_buffer = vsc.add_named_buffer(std::string("vertices"), vertices, vk::BufferUsageFlagBits::eVertexBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        index_buffer = vsc.add_named_buffer(std::string("indices"), indices, vk::BufferUsageFlagBits::eIndexBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        if (!materials.empty())
        {
            material_buffer = vsc.add_named_buffer(std::string("materials"), materials, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        }
        materials.clear();
        if (!texture_data.empty())
        {
            texture_image = vsc.add_named_image(std::string("textures"), texture_data, texture_dimensions.width, texture_dimensions.height, true, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.transfer});
        }
        texture_data.clear();
        model_render_data.resize(model_transformations.size());
        // delete vertices and indices on host
        indices.clear();
        vertices.clear();
        loaded = true;
    }

    void Scene::add_bindings()
    {
        model_render_data_buffers.push_back(vsc.add_buffer(model_render_data, vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));

        ros.at(ShaderFlavor::Default).dsh.apply_descriptor_to_new_sets(0, vsc.get_buffer(model_render_data_buffers.back()));
        if (texture_image > -1) ros.at(ShaderFlavor::Default).dsh.apply_descriptor_to_new_sets(2, vsc.get_image(texture_image));
        if (material_buffer > -1) ros.at(ShaderFlavor::Default).dsh.apply_descriptor_to_new_sets(3, vsc.get_buffer(material_buffer));
        ros.at(ShaderFlavor::Default).add_bindings(vsc);
        ros.at(ShaderFlavor::Default).dsh.reset_auto_apply_bindings();

        ros.at(ShaderFlavor::Emissive).dsh.apply_descriptor_to_new_sets(0, vsc.get_buffer(model_render_data_buffers.back()));
        if (material_buffer > -1) ros.at(ShaderFlavor::Emissive).dsh.apply_descriptor_to_new_sets(3, vsc.get_buffer(material_buffer));
        ros.at(ShaderFlavor::Emissive).add_bindings(vsc);
        ros.at(ShaderFlavor::Emissive).dsh.reset_auto_apply_bindings();

        ros.at(ShaderFlavor::Basic).dsh.apply_descriptor_to_new_sets(0, vsc.get_buffer(model_render_data_buffers.back()));
        ros.at(ShaderFlavor::Basic).add_bindings(vsc);
        ros.at(ShaderFlavor::Basic).dsh.reset_auto_apply_bindings();
    }

    void Scene::translate(const std::string& model, const glm::vec3& trans)
    {
        if (model_handles.contains(model))
        {
            model_transformations[model_handles.at(model)] = glm::translate(trans) * model_transformations[model_handles.at(model)];
        }
        else
        {
            spdlog::warn("Applying translation to not existing model!");
        }
    }

    void Scene::scale(const std::string& model, const glm::vec3& scale)
    {
        if (model_handles.contains(model))
        {
            model_transformations[model_handles.at(model)] = glm::scale(scale) * model_transformations[model_handles.at(model)];
        }
        else
        {
            spdlog::warn("Applying scale to not existing model!");
        }
    }

    void Scene::rotate(const std::string& model, float degree, const glm::vec3& axis)
    {
        if (model_handles.contains(model))
        {
            glm::mat4& transformation = model_transformations[model_handles.at(model)];
            glm::vec3 translation = transformation[3];
            transformation[3] = glm::vec4(0.0f, 0.0f, 0.0f, transformation[3].w);
            transformation = glm::rotate(glm::radians(degree), axis) * transformation;
            translate(model, translation);
        }
        else
        {
            spdlog::warn("Applying rotation to not existing model!");
        }
    }

    DescriptorSetHandler& Scene::get_dsh(ShaderFlavor flavor)
    {
        return ros.at(flavor).dsh;
    }

    void Scene::draw(vk::CommandBuffer& cb, DrawInfo& di)
    {
        vcc.graphics_cb[di.current_frame].bindVertexBuffers(0, vsc.get_buffer(vertex_buffer).get(), {0});
        vcc.graphics_cb[di.current_frame].bindIndexBuffer(vsc.get_buffer(index_buffer).get(), 0, vk::IndexType::eUint32);
        for (uint32_t i = 0; i < model_transformations.size(); ++i)
        {
            model_render_data[i].MVP = di.vp * model_transformations[i];
        }
        vsc.get_buffer(model_render_data_buffers[di.current_frame]).update_data(model_render_data);
        for (auto& ro : ros)
        {
            ro.second.draw(cb, di);
        }
    }
} // namespace ve
