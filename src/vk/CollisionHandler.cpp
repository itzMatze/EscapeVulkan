#include "vk/CollisionHandler.hpp"

#include "vk/TunnelObjects.hpp"

namespace ve
{
    CollisionHandler::CollisionHandler(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : vmc(vmc), vcc(vcc), storage(storage), compute_dsh(vmc), compute_pipeline(vmc), render_pipeline(vmc)
    {}

    void CollisionHandler::create_buffers(const std::vector<Vertex>& vertices, uint32_t scene_player_start_idx, uint32_t scene_player_idx_count)
    {
        player_start_idx = scene_player_start_idx;
        player_idx_count = scene_player_idx_count;
        bb.min = vertices[0].pos;
        bb.max = vertices[0].pos;
        for (auto& v : vertices)
        {
            bb.min = min(bb.min, v.pos);
            bb.max = max(bb.max, v.pos);
        }
        bb_buffer = storage.add_named_buffer(std::string("player_bb"), sizeof(bb), vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.compute);
        storage.get_buffer(bb_buffer).update_data(bb);
        return_buffers.push_back(storage.add_named_buffer(std::string("collision_return_0"), sizeof(int32_t), vk::BufferUsageFlagBits::eStorageBuffer, false, vmc.queue_family_indices.compute));
        return_buffers.push_back(storage.add_named_buffer(std::string("collision_return_1"), sizeof(int32_t), vk::BufferUsageFlagBits::eStorageBuffer, false, vmc.queue_family_indices.compute));
        reset_shader_return_values(0);
        reset_shader_return_values(1);
        std::vector<DebugVertex> bb_vertices(36);
        DebugVertex v0{.pos = glm::vec3(bb.min), .color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)};
        DebugVertex v1{.pos = glm::vec3(bb.max.x, bb.min.y, bb.min.z), .color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)};
        DebugVertex v2{.pos = glm::vec3(bb.max.x, bb.min.y, bb.max.z), .color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)};
        DebugVertex v3{.pos = glm::vec3(bb.min.x, bb.min.y, bb.max.z), .color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)};
        DebugVertex v4{.pos = glm::vec3(bb.max), .color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)};
        DebugVertex v5{.pos = glm::vec3(bb.min.x, bb.max.y, bb.max.z), .color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)};
        DebugVertex v6{.pos = glm::vec3(bb.min.x, bb.max.y, bb.min.z), .color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)};
        DebugVertex v7{.pos = glm::vec3(bb.max.x, bb.max.y, bb.min.z), .color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)};
        bb_vertices[0] = v0;
        bb_vertices[1] = v1;
        bb_vertices[2] = v2;
        bb_vertices[3] = v0;
        bb_vertices[4] = v2;
        bb_vertices[5] = v3;
        bb_vertices[6] = v6;
        bb_vertices[7] = v5;
        bb_vertices[8] = v4;
        bb_vertices[9] = v6;
        bb_vertices[10] = v4;
        bb_vertices[11] = v7;
        bb_vertices[12] = v1;
        bb_vertices[13] = v7;
        bb_vertices[14] = v4;
        bb_vertices[15] = v1;
        bb_vertices[16] = v4;
        bb_vertices[17] = v2;
        bb_vertices[18] = v0;
        bb_vertices[19] = v3;
        bb_vertices[20] = v5;
        bb_vertices[21] = v0;
        bb_vertices[22] = v5;
        bb_vertices[23] = v6;
        bb_vertices[24] = v0;
        bb_vertices[25] = v6;
        bb_vertices[26] = v7;
        bb_vertices[27] = v0;
        bb_vertices[28] = v7;
        bb_vertices[29] = v1;
        bb_vertices[30] = v2;
        bb_vertices[31] = v4;
        bb_vertices[32] = v5;
        bb_vertices[33] = v2;
        bb_vertices[34] = v5;
        bb_vertices[35] = v3;
        vertex_buffer = storage.add_named_buffer(std::string("player_aabb_vertices"), bb_vertices, vk::BufferUsageFlagBits::eVertexBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
    }

    void CollisionHandler::construct(const RenderPass& render_pass)
    {
        compute_dsh.add_binding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(4, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(5, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(6, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute);
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            compute_dsh.new_set();
            compute_dsh.add_descriptor(0, storage.get_buffer(bb_buffer));
            compute_dsh.add_descriptor(1, storage.get_buffer(return_buffers[i]));
            compute_dsh.add_descriptor(2, storage.get_buffer_by_name("tunnel_indices"));
            compute_dsh.add_descriptor(3, storage.get_buffer_by_name("tunnel_vertices"));
            compute_dsh.add_descriptor(4, storage.get_buffer_by_name("indices"));
            compute_dsh.add_descriptor(5, storage.get_buffer_by_name("vertices"));
            compute_dsh.add_descriptor(6, storage.get_buffer_by_name("bb_mm_" + std::to_string(i)));
        }
        compute_dsh.construct();
        construct_pipelines(render_pass);
    }

    void CollisionHandler::reload_shaders(const RenderPass& render_pass)
    {
        self_destruct(false);
        construct_pipelines(render_pass);
    }

    void CollisionHandler::construct_pipelines(const RenderPass& render_pass)
    {
        std::vector<ShaderInfo> shader_infos(2);
        shader_infos[0] = ShaderInfo{"debug.vert", vk::ShaderStageFlagBits::eVertex};
        shader_infos[1] = ShaderInfo{"debug.frag", vk::ShaderStageFlagBits::eFragment};
        std::vector<vk::PushConstantRange> pcrs;
        pcrs.push_back(vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(DebugPushConstants)));
        render_pipeline.construct(render_pass, std::nullopt, shader_infos, vk::PolygonMode::eLine, DebugVertex::get_binding_descriptions(), DebugVertex::get_attribute_descriptions(), vk::PrimitiveTopology::eTriangleList, pcrs);

        std::array<vk::SpecializationMapEntry, 7> compute_entries;
        compute_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        compute_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
        compute_entries[2] = vk::SpecializationMapEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t));
        compute_entries[3] = vk::SpecializationMapEntry(3, sizeof(uint32_t) * 3, sizeof(uint32_t));
        compute_entries[4] = vk::SpecializationMapEntry(4, sizeof(uint32_t) * 4, sizeof(uint32_t));
        compute_entries[5] = vk::SpecializationMapEntry(5, sizeof(uint32_t) * 5, sizeof(uint32_t));
        compute_entries[6] = vk::SpecializationMapEntry(6, sizeof(uint32_t) * 6, sizeof(uint32_t));
        std::array<uint32_t, 7> compute_entries_data{segment_count, samples_per_segment, vertices_per_sample, indices_per_segment, player_start_idx, player_idx_count, player_segment_position};
        vk::SpecializationInfo compute_spec_info(compute_entries.size(), compute_entries.data(), compute_entries_data.size() * sizeof(uint32_t), compute_entries_data.data());
        compute_pipeline.construct(compute_dsh.get_layouts()[0], ShaderInfo{"player_tunnel_collision.comp", vk::ShaderStageFlagBits::eCompute, compute_spec_info}, sizeof(uint32_t));
    }

    void CollisionHandler::self_destruct(bool full)
    {
        render_pipeline.self_destruct();
        compute_pipeline.self_destruct();
        if (full)
        {
            compute_dsh.self_destruct();
            storage.get_buffer(bb_buffer).self_destruct();
            for (auto& b : return_buffers) storage.get_buffer(b).self_destruct();
            return_buffers.clear();
            storage.get_buffer(vertex_buffer).self_destruct();
        }
    }

    void CollisionHandler::draw(vk::CommandBuffer& cb, GameState& gs, const glm::mat4& mvp)
    {
        // draw bounding box for debugging
        cb.bindVertexBuffers(0, storage.get_buffer(vertex_buffer).get(), {0});
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, render_pipeline.get());
        DebugPushConstants pc{.mvp = mvp};
        cb.pushConstants(render_pipeline.get_layout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(DebugPushConstants), &pc);
        cb.draw(36, 1, 0, 0);
    }

    void CollisionHandler::compute(GameState& gs, DeviceTimer& timer, uint32_t first_segment_indices_idx)
    {
        vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[gs.current_frame + frames_in_flight]);
        timer.reset(cb, {DeviceTimer::COMPUTE_PLAYER_TUNNEL_COLLISION});
        timer.start(cb, DeviceTimer::COMPUTE_PLAYER_TUNNEL_COLLISION, vk::PipelineStageFlagBits::eAllCommands);
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, compute_pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute_pipeline.get_layout(), 0, compute_dsh.get_sets()[gs.current_frame], {});
        cb.pushConstants(compute_pipeline.get_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t), &first_segment_indices_idx);
        cb.dispatch(((indices_per_segment * 2) / 3 + 31) / 32, 1, 1);
        timer.stop(cb, DeviceTimer::COMPUTE_PLAYER_TUNNEL_COLLISION, vk::PipelineStageFlagBits::eComputeShader);
        cb.end();
    }

    int32_t CollisionHandler::get_shader_return_value(uint32_t frame_idx)
    {
        return storage.get_buffer(return_buffers[frame_idx]).obtain_first_element<int32_t>();
    }

    void CollisionHandler::reset_shader_return_values(uint32_t frame_idx)
    {
        storage.get_buffer(return_buffers[frame_idx]).update_data(0);
    }
} // namespace ve
