#include "vk/TunnelObjects.hpp"

namespace ve
{
    TunnelObjects::TunnelObjects(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : vmc(vmc), vcc(vcc), storage(storage), fireflies(vmc, vcc, storage), tunnel(vmc, vcc, storage), compute_dsh(vmc), compute_pipeline(vmc), compute_normals_pipeline(vmc), tunnel_bezier_points(segment_count * 2 + 1), rnd(0), dis(0.0f, 1.0f)
    {
        cpc.indices_start_idx = 0;
    }

    void TunnelObjects::self_destruct(bool full)
    {
        compute_pipeline.self_destruct();
        compute_normals_pipeline.self_destruct();
        if (full)
        {
            storage.destroy_buffer(tunnel_bezier_points_buffer);
            fireflies.self_destruct();
            tunnel.self_destruct();
            compute_dsh.self_destruct();
        }
    }

    void TunnelObjects::create_buffers(PathTracer& path_tracer)
    {
        tunnel_bezier_points_buffer = storage.add_named_buffer(std::string("tunnel_bezier_points"), (tunnel_bezier_points.size() + 2) * 16, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.compute);
        tunnel.create_buffers();
        fireflies.create_buffers();

        compute_dsh.add_binding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            compute_dsh.new_set();
            compute_dsh.add_descriptor(0, storage.get_buffer(tunnel.index_buffer));
            compute_dsh.add_descriptor(1, storage.get_buffer(tunnel.vertex_buffer));
            compute_dsh.add_descriptor(2, storage.get_buffer(fireflies.vertex_buffers[i]));
            compute_dsh.add_descriptor(3, storage.get_buffer(tunnel_bezier_points_buffer));
        }
        compute_dsh.construct();
        construct_pipelines();

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
            cpc.indices_start_idx = i * indices_per_segment;
            const glm::vec3 normal = glm::normalize(cpc.p2 - cpc.p1);
            cpc.p1 = cpc.p2 + cpc.p2 - cpc.p1;
            cpc.p0 = cpc.p2;
            cpc.p2 = pop_tunnel_bezier_point_queue();
            tunnel_bezier_points[i * 2 + 1] = cpc.p1;
            tunnel_bezier_points[i * 2 + 2] = cpc.p2;
            compute_new_segment(cb, 1);
        }
        vcc.submit_compute(cb, true);
        vk::CommandBuffer& path_tracer_cb = vcc.begin(vcc.compute_cb[0]);
        //for (uint32_t i = 0; i < segment_count; ++i)
        {
            blas_indices.push_back(path_tracer.add_blas(path_tracer_cb, tunnel.vertex_buffer, tunnel.index_buffer, std::vector<uint32_t>{0}, std::vector<uint32_t>{index_count}, sizeof(TunnelVertex)));
            instance_indices.push_back(path_tracer.add_instance(blas_indices.back(), glm::mat4(1.0f), 666, 0xFF));
        }
        vcc.submit_compute(path_tracer_cb, true);
    }

    void TunnelObjects::construct(const RenderPass& render_pass)
    {
        fireflies.construct(render_pass);
        tunnel.construct(render_pass);
    }

    void TunnelObjects::construct_pipelines()
    {
        std::array<vk::SpecializationMapEntry, 4> compute_entries;
        compute_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        compute_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
        compute_entries[2] = vk::SpecializationMapEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t));
        compute_entries[3] = vk::SpecializationMapEntry(3, sizeof(uint32_t) * 3, sizeof(uint32_t));
        std::array<uint32_t, 4> compute_entries_data{segment_count, samples_per_segment, vertices_per_sample, fireflies_per_segment};
        vk::SpecializationInfo compute_spec_info(compute_entries.size(), compute_entries.data(), compute_entries_data.size() * sizeof(uint32_t), compute_entries_data.data());

        compute_pipeline.construct(compute_dsh.get_layouts()[0], ShaderInfo{"tunnel.comp", vk::ShaderStageFlagBits::eCompute, compute_spec_info}, sizeof(NewSegmentPushConstants));
        compute_normals_pipeline.construct(compute_dsh.get_layouts()[0], ShaderInfo{"tunnel_normals.comp", vk::ShaderStageFlagBits::eCompute, compute_spec_info}, sizeof(NewSegmentPushConstants));
    }

    void TunnelObjects::reload_shaders(const RenderPass& render_pass)
    {
        fireflies.reload_shaders(render_pass);
        tunnel.reload_shaders(render_pass);

        self_destruct(false);
        construct_pipelines();
    }

    void TunnelObjects::draw(vk::CommandBuffer& cb, GameState& gs)
    {
        fireflies.draw(cb, gs);
        tunnel.draw(cb, gs, cpc.p1, cpc.p2);
    }

    void TunnelObjects::compute_new_segment(vk::CommandBuffer& cb, uint32_t current_frame)
    {
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute_pipeline.get_layout(), 0, compute_dsh.get_sets()[current_frame], {});
        cb.pushConstants(compute_pipeline.get_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(NewSegmentPushConstants), &cpc);
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, compute_pipeline.get());
        cb.dispatch(std::max((fireflies_per_segment + 31) / 32, (vertices_per_sample * samples_per_segment + 31) / 32), 1, 1);

        Buffer& buffer = storage.get_buffer(tunnel.vertex_buffer);
        vk::BufferMemoryBarrier buffer_memory_barrier(vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eMemoryRead, vmc.queue_family_indices.compute, vmc.queue_family_indices.compute, buffer.get(), 0, buffer.get_byte_size());
        cb.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlagBits::eDeviceGroup, {}, {buffer_memory_barrier}, {});

        cb.bindPipeline(vk::PipelineBindPoint::eCompute, compute_normals_pipeline.get());
        cb.dispatch(((vertices_per_sample * samples_per_segment + 31) / 32), 1, 1);
    }

    glm::vec3 TunnelObjects::random_cosine(const glm::vec3& normal, const float cosine_weight)
    {
        float theta = std::acos(std::pow(1.0f - std::abs(dis(rnd)), 1.0f / (1.0f + cosine_weight)));
        float phi = 2.0f * M_PIf * dis(rnd);
        glm::vec3 up = abs(normal.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
        glm::vec3 bitangent = glm::cross(normal, tangent);

        glm::vec3 sample = glm::vec3(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta));
        return glm::normalize(tangent * sample.x + bitangent * sample.y + normal * sample.z);
    }

    glm::vec3 TunnelObjects::pop_tunnel_bezier_point_queue()
    {
        // no more Bézier points left, create either a long curve or a small segment
        if (tunnel_bezier_points_queue.empty())
        {
            // high probability for small segment leads to areas with small curvy segments and single long curves
            const uint32_t random_weight = dis(rnd) < 0.98f ? 1 : 16;
            glm::vec3 p2 = cpc.p0 + segment_scale * random_weight * random_cosine(glm::normalize(cpc.p1 - cpc.p0), -2.0f * random_weight + 42.0);
            glm::vec3 p1 = cpc.p0 + (cpc.p1 - cpc.p0) * float(random_weight);
            for (uint32_t i = 0; i < random_weight; ++i)
            {
                const float t = float(i + 1) / float(random_weight);
                tunnel_bezier_points_queue.push(std::pow(1 - t, 2.0f) * cpc.p0 + (2 - 2 * t) * t * p1 + std::pow(t, 2.0f) * p2);
            }
        }
        glm::vec3 p = tunnel_bezier_points_queue.front();
        tunnel_bezier_points_queue.pop();
        return p;
    }

    glm::vec3& TunnelObjects::get_tunnel_bezier_point(uint32_t segment_id, uint32_t bezier_point_idx, bool use_global_id)
    {
        // convert local id to global such that the modulo operator yields the correct idx
        if (!use_global_id) segment_id = (segment_id + cpc.segment_uid - segment_count + 1);
        return tunnel_bezier_points[(segment_id * 2 + bezier_point_idx) % tunnel_bezier_points.size()];
    }

    void TunnelObjects::advance(GameState& gs, DeviceTimer& timer, PathTracer& path_tracer)
    {
        vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[gs.current_frame]);
        FireflyMovePushConstants fmpc{.time = gs.time, .time_diff = gs.time_diff, .segment_uid = cpc.segment_uid, .first_segment_indices_idx = gs.first_segment_indices_idx};
        fireflies.move_step(cb, gs, timer, fmpc);
        if (is_pos_past_segment(gs.cam.getPosition(), player_segment_position + 1, false))
        {
            // player passed a segment, add distance of passed segment
            glm::vec3& bp0 = get_tunnel_bezier_point(player_segment_position, 0, false);
            glm::vec3& bp1 = get_tunnel_bezier_point(player_segment_position, 1, false);
            glm::vec3& bp2 = get_tunnel_bezier_point(player_segment_position, 2, false);
            // increment the idx at which the compute shader starts to compute new vertices for the corresponding indices by the number of indices in one segment
            // increment the idx at which the rendering starts by the same amount
            cpc.segment_uid++;
            cpc.indices_start_idx += indices_per_segment;
            gs.first_segment_indices_idx += indices_per_segment;

            // add new segment points
            const glm::vec3 normal = glm::normalize(cpc.p2 - cpc.p1);
            cpc.p1 = cpc.p2 + cpc.p2 - cpc.p1;
            cpc.p0 = cpc.p2;
            cpc.p2 = pop_tunnel_bezier_point_queue();
            tunnel_bezier_points[(cpc.segment_uid * 2 + 1) % tunnel_bezier_points.size()] = cpc.p1;
            tunnel_bezier_points[(cpc.segment_uid * 2 + 2) % tunnel_bezier_points.size()] = cpc.p2;
            // reset indices; compute shader inserts data at the last segment of the region that will be rendered now
            // indices need to be resetted if compute shader would write outside of the buffer or if the render region goes beyond the buffer
            // these 2 conditions are always met at the same time (as compute shader writes last rendered segment)
            if (cpc.indices_start_idx >= index_count * 2)
            {
                cpc.indices_start_idx = index_count - indices_per_segment;
                gs.first_segment_indices_idx = 0;
            }

            timer.reset(cb, {DeviceTimer::COMPUTE_TUNNEL_ADVANCE});
            timer.start(cb, DeviceTimer::COMPUTE_TUNNEL_ADVANCE, vk::PipelineStageFlagBits::eAllCommands);
            Buffer& buffer = storage.get_buffer_by_name("firefly_vertices_" + std::to_string(gs.current_frame));
            vk::BufferMemoryBarrier firefly_buffer_memory_barrier(vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eMemoryWrite, vmc.queue_family_indices.compute, vmc.queue_family_indices.compute, buffer.get(), 0, buffer.get_byte_size());
            cb.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlagBits::eDeviceGroup, {}, {firefly_buffer_memory_barrier}, {});
            compute_new_segment(cb, gs.current_frame);
            // write copy of data to the first half of the buffer if idx is in the past half of the data
            if (cpc.indices_start_idx > index_count)
            {
                cpc.indices_start_idx -= (index_count + indices_per_segment);
                compute_new_segment(cb, gs.current_frame);
                cpc.indices_start_idx += (index_count + indices_per_segment);
            }
            timer.stop(cb, DeviceTimer::COMPUTE_TUNNEL_ADVANCE, vk::PipelineStageFlagBits::eAllCommands);
            vk::BufferMemoryBarrier tunnel_buffer_memory_barrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eMemoryRead, vmc.queue_family_indices.compute, vmc.queue_family_indices.compute, storage.get_buffer(tunnel.vertex_buffer).get(), 0, storage.get_buffer(tunnel.vertex_buffer).get_byte_size());
            cb.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, vk::DependencyFlagBits::eDeviceGroup, {}, {tunnel_buffer_memory_barrier}, {});
            path_tracer.update_blas(tunnel.vertex_buffer, tunnel.index_buffer, std::vector<uint32_t>{gs.first_segment_indices_idx}, std::vector<uint32_t>{index_count}, blas_indices[0], gs.current_frame, sizeof(TunnelVertex));
        }
        path_tracer.create_tlas(cb, gs.current_frame);
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
    
    glm::vec3 TunnelObjects::get_player_reset_normal()
    {
        return glm::normalize(get_tunnel_bezier_point(player_segment_position, 1, false) - get_tunnel_bezier_point(player_segment_position, 0, false));
    }
}
