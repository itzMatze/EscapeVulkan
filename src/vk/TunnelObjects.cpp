#include "vk/TunnelObjects.hpp"

namespace ve
{
    TunnelObjects::TunnelObjects(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : vmc(vmc), vcc(vcc), storage(storage), fireflies(vmc, vcc, storage), tunnel(vmc, vcc, storage), compute_dsh(vmc), compute_pipeline(vmc), tunnel_bezier_points(segment_count * 2 + 1), rnd(0), dis(0.0f, 1.0f)
    {
        cpc.indices_start_idx = 0;
    }

    void TunnelObjects::self_destruct(bool full)
    {
        compute_pipeline.self_destruct();
        if (full)
        {
            storage.destroy_buffer(tunnel_bezier_points_buffer);
            fireflies.self_destruct();
            tunnel.self_destruct();
            compute_dsh.self_destruct();
        }
    }

    void TunnelObjects::create_buffers()
    {
        tunnel_bezier_points_buffer = storage.add_named_buffer(std::string("tunnel_bezier_points"), (tunnel_bezier_points.size() + 2) * 16, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.compute);
        tunnel.create_buffers();
        fireflies.create_buffers();
    }

    void TunnelObjects::construct(const RenderPass& render_pass)
    {
        fireflies.construct(render_pass);
        tunnel.construct(render_pass);

        compute_dsh.add_binding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            compute_dsh.new_set();
            compute_dsh.add_descriptor(0, storage.get_buffer_by_name("tunnel_indices"));
            compute_dsh.add_descriptor(1, storage.get_buffer_by_name("tunnel_vertices"));
            compute_dsh.add_descriptor(2, storage.get_buffer_by_name("firefly_vertices_" + std::to_string(i)));
            compute_dsh.add_descriptor(3, storage.get_buffer_by_name("tunnel_bezier_points"));
        }
        compute_dsh.construct();
        construct_pipelines(render_pass);

        vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[0]);
        cpc.segment_uid = 0;
        cpc.p0 = glm::vec3(0.0f, 0.0f, -50.0f);
        cpc.p1 = glm::vec3(0.0f, 0.0f, -50.0f - segment_scale / 2.0f);
        cpc.p2 = glm::vec3(0.0f, 0.0f, -50.0f - segment_scale);
        tunnel_bezier_points[0] = cpc.p0;
        tunnel_bezier_points[1] = cpc.p1;
        tunnel_bezier_points[2] = cpc.p2;
        storage.get_buffer(tunnel_bezier_points_buffer).update_data_bytes(tunnel_bezier_points.data(), 16);
        // set current_frame to 1 that fireflies are initially in buffer 1 as this is used as the in_buffer by the first frame
        compute_new_segment(cb, 1);

        for (uint32_t i = 1; i < segment_count; ++i)
        {
            cpc.segment_uid++;
            cpc.indices_start_idx = i * index_count / segment_count;
            const glm::vec3 normal = glm::normalize(cpc.p2 - cpc.p1);
            cpc.p1 = cpc.p2 + cpc.p2 - cpc.p1;
            cpc.p0 = cpc.p2;
            cpc.p2 = cpc.p0 + segment_scale * random_cosine(normal);
            tunnel_bezier_points[i * 2 + 1] = cpc.p1;
            tunnel_bezier_points[i * 2 + 2] = cpc.p2;
            compute_new_segment(cb, 1);
        }
        vcc.submit_compute(cb, true);
    }

    void TunnelObjects::construct_pipelines(const RenderPass& render_pass)
    {
        std::array<vk::SpecializationMapEntry, 4> compute_entries;
        compute_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        compute_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
        compute_entries[2] = vk::SpecializationMapEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t));
        compute_entries[3] = vk::SpecializationMapEntry(3, sizeof(uint32_t) * 3, sizeof(uint32_t));
        std::array<uint32_t, 4> compute_entries_data{segment_count, samples_per_segment, vertices_per_sample, fireflies_per_segment};
        vk::SpecializationInfo compute_spec_info(compute_entries.size(), compute_entries.data(), compute_entries_data.size() * sizeof(uint32_t), compute_entries_data.data());

        compute_pipeline.construct(compute_dsh.get_layouts()[0], ShaderInfo{"tunnel.comp", vk::ShaderStageFlagBits::eCompute, compute_spec_info}, sizeof(NewSegmentPushConstants));
    }

    void TunnelObjects::reload_shaders(const RenderPass& render_pass)
    {
        fireflies.reload_shaders(render_pass);
        tunnel.reload_shaders(render_pass);

        self_destruct(false);
        construct_pipelines(render_pass);
    }

    void TunnelObjects::draw(vk::CommandBuffer& cb, GameState& gs)
    {
        fireflies.draw(cb, gs);
        tunnel.draw(cb, gs, tunnel_render_index_start, cpc.p1, cpc.p2);
    }

    void TunnelObjects::compute_new_segment(vk::CommandBuffer& cb, uint32_t current_frame)
    {
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, compute_pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute_pipeline.get_layout(), 0, compute_dsh.get_sets()[current_frame], {});
        cb.pushConstants(compute_pipeline.get_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(NewSegmentPushConstants), &cpc);
        cb.dispatch(std::max((fireflies_per_segment + 31) / 32, (vertices_per_sample * samples_per_segment + 31) / 32), 1, 1);
    }

    glm::vec3 TunnelObjects::random_cosine(const glm::vec3& normal)
    {
        constexpr float cosine_weight = 40.0f;
        float theta = std::acos(std::pow(1.0f - std::abs(dis(rnd)), 1.0f / (1.0f + cosine_weight)));
        float phi = 2.0f * M_PIf * dis(rnd);
        glm::vec3 up = abs(normal.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
        glm::vec3 bitangent = glm::cross(normal, tangent);

        glm::vec3 sample = glm::vec3(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta));
        return glm::normalize(tangent * sample.x + bitangent * sample.y + normal * sample.z);
    }

    glm::vec3& TunnelObjects::get_tunnel_bezier_point(uint32_t segment_id, uint32_t bezier_point_idx, bool use_global_id)
    {
        // convert local id to global such that the modulo operator yields the correct idx
        if (!use_global_id) segment_id = (segment_id + cpc.segment_uid - segment_count + 1);
        return tunnel_bezier_points[(segment_id * 2 + bezier_point_idx) % tunnel_bezier_points.size()];
    }

    void TunnelObjects::advance(GameState& gs, DeviceTimer& timer)
    {
        vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[gs.current_frame]);
        FireflyMovePushConstants fmpc{.time = gs.time, .time_diff = gs.time_diff, .segment_uid = cpc.segment_uid, .first_segment_indices_idx = tunnel_render_index_start};
        fireflies.move_step(cb, gs, timer, fmpc);
        if (is_pos_past_segment(gs.player_pos, player_segment_position + 1, false))
        {
            // player passed a segment, add distance of passed segment
            glm::vec3& bp0 = get_tunnel_bezier_point(player_segment_position, 0, false);
            glm::vec3& bp1 = get_tunnel_bezier_point(player_segment_position, 1, false);
            glm::vec3& bp2 = get_tunnel_bezier_point(player_segment_position, 2, false);
            gs.tunnel_distance_travelled += glm::distance(bp0, bp1) + glm::distance(bp1, bp2);
            // increment the idx at which the compute shader starts to compute new vertices for the corresponding indices by the number of indices in one segment
            // increment the idx at which the rendering starts by the same amount
            cpc.segment_uid++;
            cpc.indices_start_idx += indices_per_segment;
            tunnel_render_index_start += indices_per_segment;

            // add new segment points
            const glm::vec3 normal = glm::normalize(cpc.p2 - cpc.p1);
            cpc.p1 = cpc.p2 + cpc.p2 - cpc.p1;
            cpc.p0 = cpc.p2;
            cpc.p2 = cpc.p2 + segment_scale * random_cosine(normal);
            tunnel_bezier_points[(cpc.segment_uid * 2 + 1) % tunnel_bezier_points.size()] = cpc.p1;
            tunnel_bezier_points[(cpc.segment_uid * 2 + 2) % tunnel_bezier_points.size()] = cpc.p2;
            // reset indices; compute shader inserts data at the last segment of the region that will be rendered now
            // indices need to be resetted if compute shader would write outside of the buffer or if the render region goes beyond the buffer
            // these 2 conditions are always met at the same time (as compute shader writes last rendered segment)
            if (cpc.indices_start_idx >= index_count * 2)
            {
                cpc.indices_start_idx = index_count - indices_per_segment;
                tunnel_render_index_start = 0;
            }

            timer.reset(cb, {DeviceTimer::COMPUTE_TUNNEL_ADVANCE});
            timer.start(cb, DeviceTimer::COMPUTE_TUNNEL_ADVANCE, vk::PipelineStageFlagBits::eAllCommands);
            Buffer& buffer = storage.get_buffer_by_name("firefly_vertices_" + std::to_string(gs.current_frame));
            vk::BufferMemoryBarrier buffer_memory_barrier(vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eMemoryWrite, vmc.queue_family_indices.compute, vmc.queue_family_indices.compute, buffer.get(), 0, buffer.get_byte_size());
            cb.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlagBits::eDeviceGroup, {}, {buffer_memory_barrier}, {});
            compute_new_segment(cb, gs.current_frame);
            // write copy of data to the first half of the buffer if idx is in the past half of the data
            if (cpc.indices_start_idx > index_count)
            {
                cpc.indices_start_idx -= (index_count + indices_per_segment);
                compute_new_segment(cb, gs.current_frame);
                cpc.indices_start_idx += (index_count + indices_per_segment);
            }
            timer.stop(cb, DeviceTimer::COMPUTE_TUNNEL_ADVANCE, vk::PipelineStageFlagBits::eAllCommands);
        }
        cb.end();
    }

    bool TunnelObjects::is_pos_past_segment(glm::vec3 pos, uint32_t idx, bool use_global_id)
    {
        return glm::dot(glm::normalize(pos - get_tunnel_bezier_point(idx, 0, use_global_id)), normalize(get_tunnel_bezier_point(idx, 1, use_global_id) - get_tunnel_bezier_point(idx, 0, use_global_id))) > 0.0f;
    }

    glm::vec3 TunnelObjects::get_player_reset_position()
    {
        return get_tunnel_bezier_point(player_segment_position, 0, false);
    }

    uint32_t TunnelObjects::get_tunnel_render_index_start() const
    {
        return tunnel_render_index_start;
    }
}
