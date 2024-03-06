#include "vk/Scene.hpp"

#include <fstream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>

#include "json.hpp"
#include "vk/Model.hpp"
#include "Camera.hpp"
#include "vk/TunnelConstants.hpp"

namespace ve
{
    Scene::Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : vmc(vmc), vcc(vcc), storage(storage), tunnel_objects(vmc, vcc, storage), collision_handler(vmc, vcc, storage), path_tracer(vmc, vcc, storage), jp(vmc, vcc, storage)
    {}

    void Scene::construct(const RenderPass& render_pass)
    {
        if (!loaded) VE_THROW("Cannot construct scene before loading one!");
        mesh_render_data_buffer = storage.add_named_buffer("mesh_render_data", mesh_render_data, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        tunnel_objects.create_buffers(path_tracer);
        jp.create_buffers();
        vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[0]);
        path_tracer.create_tlas(cb, 0);
        path_tracer.create_tlas(cb, 1);
        vcc.submit_compute(cb, true);
        // initialize tunnel
        tunnel_objects.construct(render_pass);
        collision_handler.construct(render_pass);
        // add one uniform buffer and descriptor set for each frame as the uniform buffer is changed in every frame
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            model_render_data_buffers.push_back(storage.add_buffer(model_render_data, vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));

            ros.at(ShaderFlavor::Default).dsh.new_set();
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(0, storage.get_buffer(model_render_data_buffers.back()));
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(1, storage.get_buffer_by_name("mesh_render_data"));
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(2, storage.get_image_by_name("textures"));
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(3, storage.get_buffer_by_name("materials"));
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(4, storage.get_buffer_by_name("spaceship_lights_" + std::to_string(i)));
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(5, storage.get_buffer_by_name("firefly_vertices_" + std::to_string(i)));
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(6, storage.get_image_by_name("noise_textures"));
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(10, storage.get_buffer_by_name("tunnel_indices"));
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(11, storage.get_buffer_by_name("tunnel_vertices"));
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(12, storage.get_buffer_by_name("indices"));
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(13, storage.get_buffer_by_name("vertices"));
            ros.at(ShaderFlavor::Default).dsh.add_descriptor(90, storage.get_buffer_by_name("frame_data_" + std::to_string(i)));
 
            ros.at(ShaderFlavor::Basic).dsh.new_set();
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(0, storage.get_buffer(model_render_data_buffers.back()));
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(1, storage.get_buffer_by_name("mesh_render_data"));
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(2, storage.get_image_by_name("textures"));
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(3, storage.get_buffer_by_name("materials"));
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(4, storage.get_buffer_by_name("spaceship_lights_" + std::to_string(i)));
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(5, storage.get_buffer_by_name("firefly_vertices_" + std::to_string(i)));
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(6, storage.get_image_by_name("noise_textures"));
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(10, storage.get_buffer_by_name("tunnel_indices"));
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(11, storage.get_buffer_by_name("tunnel_vertices"));
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(12, storage.get_buffer_by_name("indices"));
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(13, storage.get_buffer_by_name("vertices"));
            ros.at(ShaderFlavor::Basic).dsh.add_descriptor(90, storage.get_buffer_by_name("frame_data_" + std::to_string(i)));

            ros.at(ShaderFlavor::Emissive).dsh.new_set();
            ros.at(ShaderFlavor::Emissive).dsh.add_descriptor(0, storage.get_buffer(model_render_data_buffers.back()));
            ros.at(ShaderFlavor::Emissive).dsh.add_descriptor(1, storage.get_buffer(mesh_render_data_buffer));
            if (material_buffer > -1) ros.at(ShaderFlavor::Emissive).dsh.add_descriptor(3, storage.get_buffer(material_buffer));
            ros.at(ShaderFlavor::Emissive).dsh.add_descriptor(90, storage.get_buffer_by_name("frame_data_" + std::to_string(i)));
        }
        Mesh spawn_mesh;
        if (!ros.at(ShaderFlavor::Emissive).get_mesh("Engine_Lights", spawn_mesh)) VE_THROW("Failed to find desired spawn mesh for particles!");
        jp.construct(render_pass, spawn_mesh, model_render_data_buffers, mesh_render_data[spawn_mesh.mesh_render_data_idx].model_render_data_idx);
        construct_pipelines(render_pass, false);
    }

    void Scene::self_destruct()
    {
        path_tracer.self_destruct();
        jp.self_destruct();
        storage.destroy_buffer(vertex_buffer);
        storage.destroy_buffer(index_buffer);
        storage.destroy_buffer(mesh_render_data_buffer);
        if (material_buffer > -1) storage.destroy_buffer(material_buffer);
        material_buffer = -1;
        for (int32_t& light_buffer : light_buffers)
        {
            if (light_buffer > -1) storage.destroy_buffer(light_buffer);
            light_buffer = -1;
        }
        lights.clear();
        initial_light_values.clear();
        for (auto& b : bb_mm_buffers) storage.get_buffer(b).self_destruct();
        for (auto& b : frame_data_buffers) storage.get_buffer(b).self_destruct();
        bb_mm_buffers.clear();
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
        tunnel_objects.self_destruct();
        collision_handler.self_destruct();
    }

    void Scene::construct_pipelines(const RenderPass& render_pass, bool reload)
    {
        vk::SpecializationMapEntry model_render_data_buffer_size_entry(0, 0, sizeof(uint32_t));
        uint32_t model_render_data_buffer_size = model_render_data.size();
        vk::SpecializationInfo model_render_spec_info(1, &model_render_data_buffer_size_entry, sizeof(uint32_t), &model_render_data_buffer_size);
        std::vector<ShaderInfo> shader_infos(2);
        shader_infos[0] = ShaderInfo{"default.vert", vk::ShaderStageFlagBits::eVertex, model_render_spec_info};

        std::array<vk::SpecializationMapEntry, 3> fragment_entries;
        fragment_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        fragment_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
        fragment_entries[2] = vk::SpecializationMapEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t));
        std::array<uint32_t, 3> fragment_entries_data{uint32_t(lights.size()), segment_count, fireflies_per_segment};
        vk::SpecializationInfo fragment_spec_info(fragment_entries.size(), fragment_entries.data(), sizeof(uint32_t) * fragment_entries_data.size(), fragment_entries_data.data());

        shader_infos[1] = ShaderInfo{"default.frag", vk::ShaderStageFlagBits::eFragment, fragment_spec_info};
        ros.at(ShaderFlavor::Default).construct(render_pass, shader_infos, reload);
        shader_infos[1] = ShaderInfo{"basic.frag", vk::ShaderStageFlagBits::eFragment, fragment_spec_info};
        ros.at(ShaderFlavor::Basic).construct(render_pass, shader_infos, reload);
        shader_infos[0] = ShaderInfo("emissive.vert", vk::ShaderStageFlagBits::eVertex, model_render_spec_info);
        shader_infos[1] = ShaderInfo{"emissive.frag", vk::ShaderStageFlagBits::eFragment};
        ros.at(ShaderFlavor::Emissive).construct(render_pass, shader_infos, reload);
    }

    void Scene::reload_shaders(const RenderPass& render_pass)
    {
        construct_pipelines(render_pass, true);
        tunnel_objects.reload_shaders(render_pass);
        collision_handler.reload_shaders(render_pass);
    }

    void Scene::load(const std::string& path)
    {
        std::vector<ModelInfo> model_infos;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<std::vector<unsigned char>> texture_data;
        vk::Extent2D texture_dimensions;
        std::vector<Material> materials;

        auto add_model = [&](Model& model, const std::string& name, const glm::mat4& transformation) -> void
        {
            model_infos.push_back({});
            model_infos.back().index_buffer_idx = indices.size();
            vertices.insert(vertices.end(), model.vertices.begin(), model.vertices.end());
            indices.insert(indices.end(), model.indices.begin(), model.indices.end());
            model_infos.back().num_indices = indices.size() - model_infos.back().index_buffer_idx;
            materials.insert(materials.end(), model.materials.begin(), model.materials.end());
            lights.insert(lights.end(), model.lights.begin(), model.lights.end());
            texture_data.insert(texture_data.begin(), model.texture_data.begin(), model.texture_data.end());
            if (!model.texture_data.empty())
            {
                VE_ASSERT(texture_dimensions == model.texture_dimensions || texture_dimensions == 0, "Textures have different dimensions!");
                texture_dimensions = model.texture_dimensions;
            }
            model_render_data.push_back(ModelRenderData{.M = glm::mat4(1.0f), .segment_uid = 0});
            model_handles.emplace(name, model_render_data.size() - 1);
            model_infos.back().name = name;
            for (auto& ro : ros)
            {
                std::vector<Mesh>& meshes = model.get_mesh_list(ro.first);
                for (Mesh& mesh : meshes)
                {
                    mesh_render_data.push_back(MeshRenderData{.model_render_data_idx = int32_t(model_render_data.size() - 1), .mat_idx = mesh.material_idx, .indices_idx = mesh.index_offset});
                    mesh.mesh_render_data_idx = mesh_render_data.size() - 1;
                    model_infos.back().mesh_index_offsets.push_back(mesh.index_offset);
                    model_infos.back().mesh_index_count.push_back(mesh.index_count);
                }
                ro.second.add_model_meshes(meshes);
            }
        };

        ros.emplace(ShaderFlavor::Default, vmc);
        ros.emplace(ShaderFlavor::Basic, vmc);
        ros.emplace(ShaderFlavor::Emissive, vmc);

        ros.at(ShaderFlavor::Default).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        ros.at(ShaderFlavor::Default).dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Default).dsh.add_binding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Default).dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Default).dsh.add_binding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Default).dsh.add_binding(5, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Default).dsh.add_binding(6, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Default).dsh.add_binding(10, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Default).dsh.add_binding(11, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Default).dsh.add_binding(12, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Default).dsh.add_binding(13, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Default).dsh.add_binding(90, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);

        ros.at(ShaderFlavor::Basic).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(5, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(6, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(10, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(11, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(12, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(13, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Basic).dsh.add_binding(90, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);

        ros.at(ShaderFlavor::Emissive).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        ros.at(ShaderFlavor::Emissive).dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Emissive).dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        ros.at(ShaderFlavor::Emissive).dsh.add_binding(90, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);
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

                // apply transformations to model
                glm::mat4 transformation(1.0f);
                if (d.contains("scale"))
                {
                    transformation[0][0] = d.at("scale")[0];
                    transformation[1][1] = d.at("scale")[1];
                    transformation[2][2] = d.at("scale")[2];
                }
                if (d.contains("rotation"))
                {
                    transformation = glm::rotate(transformation, glm::radians(float(d.at("rotation")[0])), glm::vec3(d.at("rotation")[1], d.at("rotation")[2], d.at("rotation")[3]));
                }
                if (d.contains("translation"))
                {
                    transformation[3][0] = d.at("translation")[0];
                    transformation[3][1] = d.at("translation")[1];
                    transformation[3][2] = d.at("translation")[2];
                }
                model.apply_transformation(transformation);
                if (name == "Player") collision_handler.create_buffers(model.vertices, indices.size(), model.indices.size());
                add_model(model, name, transformation);
            }
        }
        // load custom models (vertices and indices directly contained in json file)
        if (data.contains("custom_models"))
        {
            for (const auto& d : data["custom_models"])
            {
                std::string name = d.value("name", "");
                Model model = ModelLoader::load(vmc, storage, d, indices.size(), vertices.size(), materials.size());
                add_model(model, name, glm::mat4(1.0f));
            }
        }
        vertex_buffer = storage.add_named_buffer(std::string("vertices"), vertices, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute);
        index_buffer = storage.add_named_buffer(std::string("indices"), indices, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute);
        vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[0]);
        for (uint32_t i = 0; i < model_infos.size(); ++i)
        {
            ModelInfo& mi = model_infos[i];
            uint32_t mask = mi.name == "Player" ? 0xFE : 0xFF;
            mi.blas_idx = path_tracer.add_blas(cb, vertex_buffer, index_buffer, mi.mesh_index_offsets, mi.mesh_index_count, sizeof(Vertex));
            mi.instance_idx = path_tracer.add_instance(mi.blas_idx, model_render_data[i].M, i, mask);
        }
        vcc.submit_compute(cb, true);
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
            light_buffers[0] = storage.add_named_buffer(std::string("spaceship_lights_0"), lights, vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
            light_buffers[1] = storage.add_named_buffer(std::string("spaceship_lights_1"), lights, vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        }
        if (!texture_data.empty())
        {
            texture_image = storage.add_named_image(std::string("textures"), texture_data, texture_dimensions.width, texture_dimensions.height, true, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eSampled);
        }
        texture_data.clear();
        // delete vertices and indices on host
        indices.clear();
        vertices.clear();

        bb_mm_buffers.push_back(storage.add_named_buffer(std::string("bb_mm_0"), sizeof(ModelMatrices), vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.compute));
        bb_mm_buffers.push_back(storage.add_named_buffer(std::string("bb_mm_1"), sizeof(ModelMatrices), vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.compute));
        frame_data_buffers.push_back(storage.add_named_buffer(std::string("frame_data_0"), sizeof(FrameData), vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.compute));
        frame_data_buffers.push_back(storage.add_named_buffer(std::string("frame_data_1"), sizeof(FrameData), vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.compute));
        FrameData frame_data;
        for (auto b : frame_data_buffers) storage.get_buffer(b).update_data(frame_data);

        loaded = true;
    }

    void Scene::translate(const std::string& model, const glm::vec3& trans)
    {
        if (model_handles.contains(model))
        {
            model_render_data[model_handles.at(model)].M = glm::translate(model_render_data[model_handles.at(model)].M, trans);
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
            model_render_data[model_handles.at(model)].M = glm::scale(model_render_data[model_handles.at(model)].M, scale);
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
            transformation = glm::rotate(transformation, glm::radians(degree), axis);
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

    void Scene::restart()
    {
        for (auto& d : model_render_data)
        {
            d.M = glm::mat4(1.0f);
            d.segment_uid = 0;
        }
        collision_handler.reset_all_shader_return_values();
        tunnel_objects.restart(path_tracer);
    }

    void Scene::draw(vk::CommandBuffer& cb, GameState& gs, DeviceTimer& timer)
    {
        uint32_t player_idx = model_handles.at("Player");
        cb.bindVertexBuffers(0, storage.get_buffer(vertex_buffer).get(), {0});
        cb.bindIndexBuffer(storage.get_buffer(index_buffer).get(), 0, vk::IndexType::eUint32);
        for (auto& ro : ros)
        {
            if (gs.game_data.show_player) ro.second.draw(cb, gs);
        }
        timer.start(cb, DeviceTimer::RENDERING_TUNNEL, vk::PipelineStageFlagBits::eAllCommands);
        tunnel_objects.draw(cb, gs);
        timer.stop(cb, DeviceTimer::RENDERING_TUNNEL, vk::PipelineStageFlagBits::eAllCommands);
        if (gs.settings.show_player_bb) collision_handler.draw(cb, model_render_data[player_idx].MVP);
        if (gs.game_data.show_player) jp.draw(cb, gs);
    }

    void Scene::update_game_state(vk::CommandBuffer& cb, GameState& gs, DeviceTimer& timer)
    {
        uint32_t player_idx = model_handles.at("Player");
        glm::mat4 vp = gs.cam.getVP();
        // let camera follow the players object
        // world position of players object is camera position, camera is 10 behind the object
        if (gs.cam.is_tracking_camera)
        {
            model_render_data[player_idx].M = glm::rotate(glm::inverse(gs.cam.view), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        }
        // get id of segment player is currently in
        gs.game_data.player_data.pos = model_render_data[player_idx].M[3];
        gs.game_data.player_data.dir = model_render_data[player_idx].M * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
        gs.game_data.player_data.up = model_render_data[player_idx].M * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        for (uint32_t j = 0; j < segment_count && tunnel_objects.is_pos_past_segment(gs.game_data.player_data.pos, model_render_data[player_idx].segment_uid + 1, true); ++j)
        {
            model_render_data[player_idx].segment_uid++;
            gs.game_data.player_data.segment_id++;
        }
        for (uint32_t i = 0; i < model_render_data.size(); ++i)
        {
            model_render_data[i].prev_MVP = model_render_data[i].MVP;
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
        path_tracer.update_instance(0, model_render_data[player_idx].M);

        ModelMatrices bb_mm{.m = model_render_data[player_idx].M, .inv_m = glm::inverse(model_render_data[player_idx].M)};
        storage.get_buffer(bb_mm_buffers[gs.game_data.current_frame]).update_data(bb_mm);
        tunnel_objects.advance(gs, timer, path_tracer);
        FrameData frame_data{gs.game_data.player_data.pos, gs.game_data.player_data.dir, gs.game_data.player_data.up, gs.game_data.player_data.segment_id, gs.game_data.time_diff, gs.game_data.time, gs.game_data.first_segment_indices_idx, gs.settings.color_view, gs.settings.normal_view, gs.settings.tex_view, gs.settings.segment_uid_view};
        storage.get_buffer(frame_data_buffers[gs.game_data.current_frame]).update_data(frame_data);
        collision_handler.compute(gs.game_data.current_frame, timer);

        if (!lights.empty()) storage.get_buffer(light_buffers[gs.game_data.current_frame]).update_data(lights);
        storage.get_buffer(model_render_data_buffers[gs.game_data.current_frame]).update_data(model_render_data);
        // handle collision: reset ship and let it blink for 3s
        gs.game_data.collision_results = collision_handler.get_collision_results(gs.game_data.current_frame);
        // check if player tries to move in the wrong direction
        if (!tunnel_objects.is_pos_past_segment(gs.game_data.player_data.pos, std::max(gs.game_data.player_data.segment_id - 1, 0u), true)) gs.game_data.collision_results.collision_detected = 1;
        if (gs.game_data.player_reset_blink_counter == 0 && gs.game_data.collision_results.collision_detected != 0 && gs.settings.collision_detection_active)
        {
            gs.cam.position = tunnel_objects.get_player_reset_position();
            gs.game_data.player_lifes--;

            gs.game_data.player_reset_blink_counter = 6;
            gs.game_data.player_reset_blink_timer = 0.5f;
        }
        if (gs.game_data.player_reset_blink_counter > 0)
        {
            collision_handler.reset_shader_return_values(gs.game_data.current_frame);
            gs.game_data.player_reset_blink_timer -= gs.game_data.time_diff;
            gs.cam.position = tunnel_objects.get_player_reset_position();
            glm::vec3 normal = tunnel_objects.get_player_reset_normal();
            if (std::abs(glm::dot(normal, glm::vec3(1.0f, 0.0f, 0.0f))) > 0.999f)
            {
                gs.cam.orientation = glm::quatLookAt(normal, glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));
            }
            else
            {
                gs.cam.orientation = glm::quatLookAt(normal, glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f)));
            }
            if (gs.game_data.player_reset_blink_timer < 0.0f)
            {
                gs.game_data.player_reset_blink_counter--;
                gs.game_data.player_reset_blink_timer = 0.5f;
                gs.game_data.show_player = !gs.game_data.show_player;
            }
        }
        jp.move_step(cb, gs.game_data.current_frame);
    }

    uint32_t Scene::get_light_count()
    {
        return lights.size();
    }
} // namespace ve
