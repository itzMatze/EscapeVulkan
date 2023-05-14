#include "vk/Fireflies.hpp"
#include "vk/TunnelObjects.hpp"

namespace ve
{
    Fireflies::Fireflies(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : render_dsh(vmc), compute_dsh(vmc), vmc(vmc), vcc(vcc), storage(storage), pipeline(vmc), compute_pipeline(vmc)
    {}

    void Fireflies::self_destruct(bool full)
    {
        pipeline.self_destruct();
        compute_pipeline.self_destruct();
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

    void Fireflies::construct(const RenderPass& render_pass, uint32_t parallel_units)
    {
        std::vector<FireflyVertex> vertices(firefly_count);
        vertex_buffers.push_back(storage.add_named_buffer(std::string("firefly_vertices_0"), vertices, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));
        vertex_buffers.push_back(storage.add_named_buffer(std::string("firefly_vertices_1"), vertices, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));

        render_dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        compute_dsh.add_binding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);

        // add one uniform buffer and descriptor set for each frame as the uniform buffer is changed in every frame
        for (uint32_t i = 0; i < parallel_units; ++i)
        {
            model_render_data_buffers.push_back(storage.add_buffer(std::vector<ModelRenderData>{mrd}, vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));

            render_dsh.apply_descriptor_to_new_sets(0, storage.get_buffer(model_render_data_buffers.back()));
            render_dsh.new_set();
            render_dsh.reset_auto_apply_bindings();

            // ping-pong with firefly buffers to avoid data races
            compute_dsh.apply_descriptor_to_new_sets(0, storage.get_buffer(vertex_buffers[1 - i]));
            compute_dsh.apply_descriptor_to_new_sets(1, storage.get_buffer(vertex_buffers[i]));
            compute_dsh.new_set();
            compute_dsh.reset_auto_apply_bindings();
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
        pipeline.construct(render_pass, render_dsh.get_layouts()[0], shader_infos, vk::PolygonMode::ePoint, FireflyVertex::get_binding_descriptions(), FireflyVertex::get_attribute_descriptions(), vk::PrimitiveTopology::ePointList);

        std::array<vk::SpecializationMapEntry, 3> compute_entries;
        compute_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        compute_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
        compute_entries[2] = vk::SpecializationMapEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t));
        std::array<uint32_t, 3> compute_entries_data{samples_per_segment, vertices_per_sample, firefly_count};
        vk::SpecializationInfo compute_spec_info(compute_entries.size(), compute_entries.data(), compute_entries_data.size() * sizeof(uint32_t), compute_entries_data.data());

        compute_pipeline.construct(compute_dsh.get_layouts()[0], ShaderInfo{"fireflies_move.comp", vk::ShaderStageFlagBits::eCompute, compute_spec_info}, sizeof(FireflyMovePushConstants));
    }

    void Fireflies::reload_shaders(const RenderPass& render_pass)
    {
        self_destruct(false);
        construct_pipelines(render_pass);
    }

    void Fireflies::draw(vk::CommandBuffer& cb, DrawInfo& di)
    {
        cb.bindVertexBuffers(0, storage.get_buffer(vertex_buffers[di.current_frame]).get(), {0});
        mrd.MVP = di.cam.getVP();
        mrd.M = di.cam.getV();
        storage.get_buffer(model_render_data_buffers[di.current_frame]).update_data(std::vector<ModelRenderData>{mrd});
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.get_layout(), 0, render_dsh.get_sets()[di.current_frame], {});
        PushConstants pc{.mvp_idx = 0, .mat_idx = -1, .light_count = di.light_count, .time = di.time, .normal_view = di.normal_view, .tex_view = di.tex_view};
        cb.pushConstants(pipeline.get_layout(), vk::ShaderStageFlagBits::eVertex, 0, PushConstants::get_vertex_push_constant_size(), &pc);
        cb.pushConstants(pipeline.get_layout(), vk::ShaderStageFlagBits::eFragment, PushConstants::get_fragment_push_constant_offset(), PushConstants::get_fragment_push_constant_size(), pc.get_fragment_push_constant_pointer());
        cb.draw(firefly_count, 1, 0, 0);
    }

    void Fireflies::move_step(const DrawInfo& di, DeviceTimer& timer)
    {
        vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[di.current_frame + frames_in_flight]);
        timer.reset(cb, {DeviceTimer::FIREFLY_MOVE_STEP});
        timer.start(cb, DeviceTimer::FIREFLY_MOVE_STEP, vk::PipelineStageFlagBits::eAllCommands);
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, compute_pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute_pipeline.get_layout(), 0, compute_dsh.get_sets()[di.current_frame], {});
        FireflyMovePushConstants fmpc{.time = di.time, .time_diff = di.time_diff};
        cb.pushConstants(compute_pipeline.get_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(FireflyMovePushConstants), &fmpc);
        cb.dispatch((firefly_count + 31) / 32, 1, 1);
        timer.stop(cb, DeviceTimer::FIREFLY_MOVE_STEP, vk::PipelineStageFlagBits::eAllCommands);
        cb.end();
    }
} // namespace ve
