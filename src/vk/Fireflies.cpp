#include "vk/Fireflies.hpp"
#include "vk/TunnelObjects.hpp"

namespace ve
{
    Fireflies::Fireflies(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : render_dsh(vmc), compute_dsh(vmc), vmc(vmc), vcc(vcc), storage(storage), render_pipeline(vmc), move_compute_pipeline(vmc), tunnel_collision_compute_pipeline(vmc)
    {}

    void Fireflies::self_destruct(bool full)
    {
        render_pipeline.self_destruct();
        move_compute_pipeline.self_destruct();
        tunnel_collision_compute_pipeline.self_destruct();
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

    void Fireflies::create_buffers()
    {
        std::vector<FireflyVertex> vertices(firefly_count);
        vertex_buffers.push_back(storage.add_named_buffer(std::string("firefly_vertices_0"), vertices, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute));
        vertex_buffers.push_back(storage.add_named_buffer(std::string("firefly_vertices_1"), vertices, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics, vmc.queue_family_indices.compute));
    }

    void Fireflies::construct(const RenderPass& render_pass)
    {
        render_dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        compute_dsh.add_binding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(4, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(5, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(6, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(7, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute);

        // add one uniform buffer and descriptor set for each frame as the uniform buffer is changed in every frame
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            model_render_data_buffers.push_back(storage.add_buffer(std::vector<ModelRenderData>{mrd}, vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));

            render_dsh.new_set();
            render_dsh.add_descriptor(0, storage.get_buffer(model_render_data_buffers.back()));

            // ping-pong with firefly buffers to avoid data races
            compute_dsh.new_set();
            compute_dsh.add_descriptor(0, storage.get_buffer(vertex_buffers[1 - i]));
            compute_dsh.add_descriptor(1, storage.get_buffer(vertex_buffers[i]));
            compute_dsh.add_descriptor(3, storage.get_buffer_by_name("tunnel_bezier_points"));
            compute_dsh.add_descriptor(4, storage.get_buffer_by_name("tunnel_indices"));
            compute_dsh.add_descriptor(5, storage.get_buffer_by_name("tunnel_vertices"));
            compute_dsh.add_descriptor(6, storage.get_buffer_by_name("player_bb"));
            compute_dsh.add_descriptor(7, storage.get_buffer_by_name("bb_mm_" + std::to_string(i)));
        }
        render_dsh.construct();
        compute_dsh.construct();

        construct_pipelines(render_pass);
    }

    void Fireflies::construct_pipelines(const RenderPass& render_pass)
    {
        std::array<vk::SpecializationMapEntry, 1> render_entries;
        render_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        std::array<uint32_t, 1> render_entries_data{1};
        vk::SpecializationInfo render_spec_info(render_entries.size(), render_entries.data(), sizeof(uint32_t) * render_entries_data.size(), render_entries_data.data());
        std::vector<ShaderInfo> shader_infos(2);
        shader_infos[0] = ShaderInfo{"fireflies.vert", vk::ShaderStageFlagBits::eVertex, render_spec_info};
        shader_infos[1] = ShaderInfo{"fireflies.frag", vk::ShaderStageFlagBits::eFragment};
        render_pipeline.construct(render_pass, render_dsh.get_layouts()[0], shader_infos, vk::PolygonMode::ePoint, FireflyVertex::get_binding_descriptions(), FireflyVertex::get_attribute_descriptions(), vk::PrimitiveTopology::ePointList);

        std::array<vk::SpecializationMapEntry, 6> compute_entries;
        compute_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        compute_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
        compute_entries[2] = vk::SpecializationMapEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t));
        compute_entries[3] = vk::SpecializationMapEntry(3, sizeof(uint32_t) * 3, sizeof(uint32_t));
        compute_entries[4] = vk::SpecializationMapEntry(4, sizeof(uint32_t) * 4, sizeof(uint32_t));
        compute_entries[5] = vk::SpecializationMapEntry(5, sizeof(uint32_t) * 5, sizeof(uint32_t));
        std::array<uint32_t, 6> compute_entries_data{segment_count, samples_per_segment, vertices_per_sample, fireflies_per_segment, firefly_count, indices_per_segment};
        vk::SpecializationInfo compute_spec_info(compute_entries.size(), compute_entries.data(), compute_entries_data.size() * sizeof(uint32_t), compute_entries_data.data());

        move_compute_pipeline.construct(compute_dsh.get_layouts()[0], ShaderInfo{"fireflies_move.comp", vk::ShaderStageFlagBits::eCompute, compute_spec_info}, sizeof(FireflyMovePushConstants));
        tunnel_collision_compute_pipeline.construct(compute_dsh.get_layouts()[0], ShaderInfo{"fireflies_tunnel_collision.comp", vk::ShaderStageFlagBits::eCompute, compute_spec_info}, sizeof(FireflyMovePushConstants));
    }

    void Fireflies::reload_shaders(const RenderPass& render_pass)
    {
        self_destruct(false);
        construct_pipelines(render_pass);
    }

    void Fireflies::draw(vk::CommandBuffer& cb, GameState& gs)
    {
        cb.bindVertexBuffers(0, storage.get_buffer(vertex_buffers[gs.current_frame]).get(), {0});
        mrd.MVP = gs.cam.getVP();
        mrd.M = gs.cam.getV();
        storage.get_buffer(model_render_data_buffers[gs.current_frame]).update_data(std::vector<ModelRenderData>{mrd});
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, render_pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, render_pipeline.get_layout(), 0, render_dsh.get_sets()[gs.current_frame], {});
        PushConstants pc{.mvp_idx = 0, .mat_idx = -1, .time = gs.time, .normal_view = gs.normal_view, .tex_view = gs.tex_view};
        cb.pushConstants(render_pipeline.get_layout(), vk::ShaderStageFlagBits::eVertex, 0, PushConstants::get_vertex_push_constant_size(), &pc);
        cb.pushConstants(render_pipeline.get_layout(), vk::ShaderStageFlagBits::eFragment, PushConstants::get_fragment_push_constant_offset(), PushConstants::get_fragment_push_constant_size(), pc.get_fragment_push_constant_pointer());
        cb.draw(firefly_count, 1, 0, 0);
    }

    void Fireflies::move_step(vk::CommandBuffer& cb, const GameState& gs, DeviceTimer& timer, FireflyMovePushConstants& fmpc)
    {
        timer.reset(cb, {DeviceTimer::FIREFLY_MOVE_STEP});
        timer.start(cb, DeviceTimer::FIREFLY_MOVE_STEP, vk::PipelineStageFlagBits::eAllCommands);
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, move_compute_pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, move_compute_pipeline.get_layout(), 0, compute_dsh.get_sets()[gs.current_frame], {});
        cb.pushConstants(move_compute_pipeline.get_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(FireflyMovePushConstants), &fmpc);
        cb.dispatch((firefly_count + 31) / 32, 1, 1);
        Buffer& buffer = storage.get_buffer(vertex_buffers[gs.current_frame]);
        vk::BufferMemoryBarrier buffer_memory_barrier(vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eMemoryRead, vmc.queue_family_indices.compute, vmc.queue_family_indices.compute, buffer.get(), 0, buffer.get_byte_size());
        cb.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlagBits::eDeviceGroup, {}, {buffer_memory_barrier}, {});
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, tunnel_collision_compute_pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, tunnel_collision_compute_pipeline.get_layout(), 0, compute_dsh.get_sets()[gs.current_frame], {});
        cb.pushConstants(tunnel_collision_compute_pipeline.get_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(FireflyMovePushConstants), &fmpc);
        cb.dispatch((firefly_count + 31) / 32, ((indices_per_segment / 3) + 31) / 32, 1);
        timer.stop(cb, DeviceTimer::FIREFLY_MOVE_STEP, vk::PipelineStageFlagBits::eComputeShader);
    }
} // namespace ve
