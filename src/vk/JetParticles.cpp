#include "vk/JetParticles.hpp"
#include "Camera.hpp"
#include "vk/gpu_data/JetParticlesGpuData.hpp"

namespace ve
{
    constexpr uint32_t jet_particle_count = 20000;

    JetParticles::JetParticles(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : render_dsh(vmc), compute_dsh(vmc), vmc(vmc), vcc(vcc), storage(storage), render_pipeline(vmc), move_compute_pipeline(vmc)
    {}

    void JetParticles::self_destruct(bool full)
    {
        render_pipeline.self_destruct();
        move_compute_pipeline.self_destruct();
        if (full)
        {
            render_dsh.self_destruct();
            compute_dsh.self_destruct();
            for (auto i : vertex_buffers) storage.destroy_buffer(i);
            vertex_buffers.clear();
            for (auto i : model_render_data_buffers) storage.destroy_buffer(i);
            model_render_data_buffers.clear();
        }
    }

    void JetParticles::create_buffers()
    {
        std::vector<JetParticleVertex> vertices(jet_particle_count, JetParticleVertex{.pos = glm::vec3(0.0), .col = glm::vec3(1.0f, 0.0f, 1.0f), .vel = glm::vec3(0.0f, 10.0f, 0.0f), .lifetime = 0.0f});
        vertex_buffers.push_back(storage.add_named_buffer(std::string("jet_particle_vertices_0"), vertices, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute));
        vertex_buffers.push_back(storage.add_named_buffer(std::string("jet_particle_vertices_1"), vertices, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute));
    }

    void JetParticles::construct(const RenderPass& render_pass, const Mesh& spawn_mesh, const std::vector<uint32_t>& spawn_mesh_model_render_data_buffer, uint32_t spawn_mesh_model_render_data_idx)
    {
        spawn_mesh_model_render_data_buffer_count = storage.get_buffer(spawn_mesh_model_render_data_buffer[0]).get_element_count();
        spawn_mesh_model_render_data_buffer_idx = spawn_mesh_model_render_data_idx;
        mesh = spawn_mesh;
        render_dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        compute_dsh.add_binding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute);

        // add one uniform buffer and descriptor set for each frame as the uniform buffer is changed in every frame
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            model_render_data_buffers.push_back(storage.add_buffer(std::vector<ModelRenderData>{mrd}, vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));

            render_dsh.new_set();
            render_dsh.add_descriptor(0, storage.get_buffer(model_render_data_buffers.back()));

            compute_dsh.new_set();
            compute_dsh.add_descriptor(0, storage.get_buffer(vertex_buffers[1 - i]));
            compute_dsh.add_descriptor(1, storage.get_buffer(vertex_buffers[i]));
            compute_dsh.add_descriptor(2, storage.get_buffer_by_name("indices"));
            compute_dsh.add_descriptor(3, storage.get_buffer_by_name("vertices"));
            compute_dsh.add_descriptor(4, storage.get_buffer(spawn_mesh_model_render_data_buffer[i]));
        }
        render_dsh.construct();
        compute_dsh.construct();

        construct_pipelines(render_pass);
    }

    void JetParticles::construct_pipelines(const RenderPass& render_pass)
    {
        std::array<vk::SpecializationMapEntry, 2> vertex_entries;
        vertex_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(float));
        vertex_entries[1] = vk::SpecializationMapEntry(1, sizeof(float), sizeof(float));
        std::array<float, 2> vertex_entries_data{40.0f, max_particle_lifetime};
        vk::SpecializationInfo vertex_spec_info(vertex_entries.size(), vertex_entries.data(), sizeof(uint32_t) * vertex_entries_data.size(), vertex_entries_data.data());

        std::vector<ShaderInfo> shader_infos(2);
        shader_infos[0] = ShaderInfo{"jet_particles.vert", vk::ShaderStageFlagBits::eVertex, vertex_spec_info};
        shader_infos[1] = ShaderInfo{"jet_particles.frag", vk::ShaderStageFlagBits::eFragment};
        render_pipeline.construct(render_pass, render_dsh.get_layouts()[0], shader_infos, vk::PolygonMode::ePoint, JetParticleVertex::get_binding_descriptions(), JetParticleVertex::get_attribute_descriptions(), vk::PrimitiveTopology::ePointList, {});

        std::array<vk::SpecializationMapEntry, 6> compute_entries;
        compute_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        compute_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
        compute_entries[2] = vk::SpecializationMapEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t));
        compute_entries[3] = vk::SpecializationMapEntry(3, sizeof(uint32_t) * 3, sizeof(uint32_t));
        compute_entries[4] = vk::SpecializationMapEntry(4, sizeof(uint32_t) * 4, sizeof(uint32_t));
        compute_entries[5] = vk::SpecializationMapEntry(5, sizeof(uint32_t) * 5, sizeof(float));
        float lifetime = max_particle_lifetime;
        std::array<uint32_t, 6> compute_entries_data{jet_particle_count, mesh.index_offset, mesh.index_count, spawn_mesh_model_render_data_buffer_count, spawn_mesh_model_render_data_buffer_idx, *reinterpret_cast<uint32_t*>(&lifetime)};
        vk::SpecializationInfo compute_spec_info(compute_entries.size(), compute_entries.data(), compute_entries_data.size() * sizeof(uint32_t), compute_entries_data.data());

        move_compute_pipeline.construct(compute_dsh.get_layouts()[0], ShaderInfo{"jet_particles_move.comp", vk::ShaderStageFlagBits::eCompute, compute_spec_info}, sizeof(JetParticleMovePushConstants));
    }

    void JetParticles::reload_shaders(const RenderPass& render_pass)
    {
        self_destruct(false);
        construct_pipelines(render_pass);
    }

    void JetParticles::draw(vk::CommandBuffer& cb, GameState& gs)
    {
        cb.bindVertexBuffers(0, storage.get_buffer(vertex_buffers[gs.game_data.current_frame]).get(), {0});
        mrd.prev_MVP = mrd.MVP;
        mrd.MVP = gs.cam.getVP();
        storage.get_buffer(model_render_data_buffers[gs.game_data.current_frame]).update_data(std::vector<ModelRenderData>{mrd});
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, render_pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_pipeline.get_layout(), 0, render_dsh.get_sets()[gs.game_data.current_frame], {});
        cb.draw(jet_particle_count, 1, 0, 0);
    }

    void JetParticles::move_step(vk::CommandBuffer& cb, const GameState& gs)
    {
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, move_compute_pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, move_compute_pipeline.get_layout(), 0, compute_dsh.get_sets()[gs.game_data.current_frame], {});
        JetParticleMovePushConstants jpmpc{.move_dir = gs.cam.getFront(), .time = gs.game_data.time, .time_diff = gs.game_data.time_diff};
        cb.pushConstants(move_compute_pipeline.get_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(JetParticleMovePushConstants), &jpmpc);
        cb.dispatch((jet_particle_count + 31) / 32, 1, 1);
    }
} // namespace ve
