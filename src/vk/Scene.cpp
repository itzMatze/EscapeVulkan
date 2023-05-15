#include "vk/Scene.hpp"

#include <fstream>
#include <glm/gtx/transform.hpp>

#include "json.hpp"

namespace ve
{
    Scene::Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : vmc(vmc), vcc(vcc), storage(storage)
    {}

    void Scene::construct(const RenderPass& render_pass)
    {
        // add one uniform buffer and descriptor set for each frame as the uniform buffer is changed in every frame
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            model_render_data_buffers.push_back(storage.add_buffer(model_render_data, vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));

            ros.at(ShaderFlavor::Default).dsh.new_set();
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(0, storage.get_buffer(model_render_data_buffers.back()));
            if (texture_image > -1) ros.at(ShaderFlavor::Default).dsh.add_descriptor(2, storage.get_image(texture_image));
            if (material_buffer > -1) ros.at(ShaderFlavor::Default).dsh.add_descriptor(3, storage.get_buffer(material_buffer));
            if (light_buffer > -1) ros.at(ShaderFlavor::Default).dsh.add_descriptor(4, storage.get_buffer(light_buffer));

            ros.at(ShaderFlavor::Emissive).dsh.new_set();
            ros.at(ShaderFlavor::Emissive).dsh.add_descriptor(0, storage.get_buffer(model_render_data_buffers.back()));
            if (material_buffer > -1) ros.at(ShaderFlavor::Emissive).dsh.add_descriptor(3, storage.get_buffer(material_buffer));

            ros.at(ShaderFlavor::Basic).dsh.new_set();
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(0, storage.get_buffer(model_render_data_buffers.back()));
            if (light_buffer > -1) ros.at(ShaderFlavor::Basic).dsh.add_descriptor(4, storage.get_buffer(light_buffer));
        }
        construct_pipelines(render_pass, false);
    }

    void Scene::self_destruct()
    {
        storage.destroy_buffer(vertex_buffer);
        storage.destroy_buffer(index_buffer);
        if (material_buffer > -1) storage.destroy_buffer(material_buffer);
        material_buffer = -1;
        if (light_buffer > -1) storage.destroy_buffer(light_buffer);
        light_buffer = -1;
        lights.clear();
        initial_light_values.clear();
        for (auto& buffer : model_render_data_buffers) storage.destroy_buffer(buffer);
        model_render_data_buffers.clear();
        model_render_data.clear();
        if (texture_image > -1) storage.destroy_image(texture_image);
        texture_image = -1;
        for (auto& ro : ros) ro.second.self_destruct();
        ros.clear(); 
        model_handles.clear();
        model_render_data.clear();
        loaded = false;
    }

    void Scene::construct_pipelines(const RenderPass& render_pass, bool reload)
    {
        vk::SpecializationMapEntry model_render_data_buffer_size_entry(0, 0, sizeof(uint32_t));
        uint32_t model_render_data_buffer_size = model_render_data.size();
        vk::SpecializationInfo model_render_spec_info(1, &model_render_data_buffer_size_entry, sizeof(uint32_t), &model_render_data_buffer_size);
        std::vector<ShaderInfo> shader_infos(2);
        shader_infos[0] = ShaderInfo{"default.vert", vk::ShaderStageFlagBits::eVertex, model_render_spec_info};

        vk::SpecializationMapEntry lights_buffer_size_entry(0, 0, sizeof(uint32_t));
        uint32_t lights_buffer_size = lights.size();
        vk::SpecializationInfo lights_spec_info(1, &lights_buffer_size_entry, sizeof(uint32_t), &lights_buffer_size);
        shader_infos[1] = ShaderInfo{"default.frag", vk::ShaderStageFlagBits::eFragment, lights_spec_info};
        ros.at(ShaderFlavor::Default).construct(render_pass, shader_infos, reload);
        shader_infos[1] = ShaderInfo{"basic.frag", vk::ShaderStageFlagBits::eFragment, lights_spec_info};
        ros.at(ShaderFlavor::Basic).construct(render_pass, shader_infos, reload);
        shader_infos[1] = ShaderInfo{"emissive.frag", vk::ShaderStageFlagBits::eFragment};
        ros.at(ShaderFlavor::Emissive).construct(render_pass, shader_infos, reload);
    }

    void Scene::reload_shaders(const RenderPass& render_pass)
    {
        construct_pipelines(render_pass, true);
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
            lights.insert(lights.end(), model.lights.begin(), model.lights.end());
            texture_data.insert(texture_data.begin(), model.texture_data.begin(), model.texture_data.end());
            if (!model.texture_data.empty())
            {
                VE_ASSERT(texture_dimensions == model.texture_dimensions || texture_dimensions == 0, "Textures have different dimensions!");
                texture_dimensions = model.texture_dimensions;
            }
            model_render_data.push_back(ModelRenderData{.M = glm::mat4(1.0f)});
            model_handles.emplace(name, model_render_data.size() - 1);
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
        ros.at(ShaderFlavor::Default).dsh.add_binding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);

        ros.at(ShaderFlavor::Basic).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);

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
                Model model = ModelLoader::load(vmc, storage, std::string("../assets/models/") + std::string(d.value("file", "")), indices.size(), vertices.size(), materials.size(), texture_data.size());
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
                Model model = ModelLoader::load(vmc, storage, d, indices.size(), vertices.size(), materials.size());
                add_model(model, name);
            }
        }
        vertex_buffer = storage.add_named_buffer(std::string("vertices"), vertices, vk::BufferUsageFlagBits::eVertexBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        index_buffer = storage.add_named_buffer(std::string("indices"), indices, vk::BufferUsageFlagBits::eIndexBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        if (!materials.empty())
        {
            material_buffer = storage.add_named_buffer(std::string("materials"), materials, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        }
        materials.clear();
        if (!lights.empty())
        {
            for (const auto& light : lights)
            {
                initial_light_values.push_back(std::make_pair(light.pos, light.dir));
            }
            light_buffer = storage.add_named_buffer(std::string("spaceship_lights"), lights, vk::BufferUsageFlagBits::eUniformBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        }

        if (!texture_data.empty())
        {
            texture_image = storage.add_named_image(std::string("textures"), texture_data, texture_dimensions.width, texture_dimensions.height, true, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eSampled);
        }
        texture_data.clear();
        // delete vertices and indices on host
        indices.clear();
        vertices.clear();
        loaded = true;
    }

    void Scene::translate(const std::string& model, const glm::vec3& trans)
    {
        if (model_handles.contains(model))
        {
            model_render_data[model_handles.at(model)].M = glm::translate(trans) * model_render_data[model_handles.at(model)].M;
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
            model_render_data[model_handles.at(model)].M = glm::scale(scale) * model_render_data[model_handles.at(model)].M;
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
            glm::mat4& transformation = model_render_data[model_handles.at(model)].M;
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
        uint32_t player_idx = model_handles.at("Player");
        glm::mat4 vp = di.cam.getVP();
        // let camera follow the players object
        // world position of players object is camera position, camera is 10 behind the object
        if (di.cam.is_tracking_camera)
        {
            model_render_data[player_idx].M = glm::rotate(glm::inverse(di.cam.view), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        di.light_count = lights.size();
        vcc.graphics_cb[di.current_frame].bindVertexBuffers(0, storage.get_buffer(vertex_buffer).get(), {0});
        vcc.graphics_cb[di.current_frame].bindIndexBuffer(storage.get_buffer(index_buffer).get(), 0, vk::IndexType::eUint32);
        for (uint32_t i = 0; i < model_render_data.size(); ++i)
        {
            model_render_data[i].MVP = vp * model_render_data[i].M;
        }
        // update lights with current position of player object
        for (uint32_t i = 0; i < lights.size(); ++i) 
        {
            glm::vec4 tmp_pos = model_render_data[player_idx].M * glm::vec4(initial_light_values[i].first, 1.0f);
            tmp_pos.x /= tmp_pos.w;
            tmp_pos.y /= tmp_pos.w;
            tmp_pos.z /= tmp_pos.w;
            lights[i].pos = glm::vec3(tmp_pos);
            glm::vec4 tmp_dir = model_render_data[player_idx].M * glm::vec4(initial_light_values[i].second, 0.0f);
            lights[i].dir = glm::vec3(tmp_dir);
        }

        if (!lights.empty()) storage.get_buffer(light_buffer).update_data(lights);
        storage.get_buffer(model_render_data_buffers[di.current_frame]).update_data(model_render_data);
        for (auto& ro : ros)
        {
            ro.second.draw(cb, di);
        }
    }
} // namespace ve
