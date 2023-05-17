#include "vk/TunnelObjects.hpp"

namespace ve
{
    TunnelObjects::TunnelObjects(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : vmc(vmc), vcc(vcc), storage(storage), fireflies(vmc, vcc, storage), tunnel(vmc, vcc, storage), compute_dsh(vmc), compute_pipeline(vmc), segment_planes(segment_count), rnd(0), dis(0.0f, 1.0f)
    {
        cpc.indices_start_idx = 0;
    }

    void TunnelObjects::self_destruct(bool full)
    {
        compute_pipeline.self_destruct();
        if (full)
        {
            fireflies.self_destruct();
            tunnel.self_destruct();
            compute_dsh.self_destruct();
        }
    }

    void TunnelObjects::construct(const RenderPass& render_pass)
    {
        fireflies.construct(render_pass);
        tunnel.construct(render_pass);

        compute_dsh.add_binding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            compute_dsh.new_set();
            compute_dsh.add_descriptor(0, storage.get_buffer_by_name("tunnel_indices"));
            compute_dsh.add_descriptor(1, storage.get_buffer_by_name("tunnel_vertices"));
            compute_dsh.add_descriptor(2, storage.get_buffer_by_name("firefly_vertices_" + std::to_string(i)));
        }
        compute_dsh.construct();
        construct_pipelines(render_pass);

        vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[0]);
        cpc.segment_idx = 0;
        cpc.p0 = glm::vec3(0.0f, 0.0f, -50.0f);
        cpc.p1 = glm::vec3(0.0f, 0.0f, -50.0f - segment_scale / 2.0f);
        cpc.p2 = glm::vec3(0.0f, 0.0f, -50.0f - segment_scale);
        segment_planes.push_back(SegmentPlane{cpc.p0, glm::normalize(cpc.p1 - cpc.p0)});
        // set current_frame to 1 that fireflies are initially in buffer 1 as this is used as the in_buffer by the first frame
        compute_new_segment(cb, 1);

        for (uint32_t i = 1; i < segment_count; ++i)
        {
            cpc.segment_idx++;
            cpc.indices_start_idx = i * index_count / segment_count;
            const glm::vec3 normal = glm::normalize(cpc.p2 - cpc.p1);
            segment_planes.push_back(SegmentPlane{cpc.p2, normal});
            cpc.p1 = cpc.p2 + cpc.p2 - cpc.p1;
            cpc.p0 = cpc.p2;
            cpc.p2 = cpc.p0 + segment_scale * random_cosine(normal);
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

    void TunnelObjects::draw(vk::CommandBuffer& cb, DrawInfo& di)
    {
        fireflies.draw(cb, di);
        tunnel.draw(cb, di, tunnel_render_index_start);
    }

    void TunnelObjects::compute_new_segment(vk::CommandBuffer& cb, uint32_t current_frame)
    {
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, compute_pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute_pipeline.get_layout(), 0, compute_dsh.get_sets()[current_frame], {});
        cb.pushConstants(compute_pipeline.get_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(NewSegmentPushConstants), &cpc);
        cb.dispatch((vertices_per_sample * samples_per_segment + 31) / 32, 1, 1);
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

    void TunnelObjects::advance(const DrawInfo& di, DeviceTimer& timer)
    {
        vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[di.current_frame]);
        fireflies.move_step(cb, di, timer);
        if (is_pos_past_segment(di.player_pos, 2, false))
        {
            // add new segment points
            const glm::vec3 normal = glm::normalize(cpc.p2 - cpc.p1);
            segment_planes.push_back(SegmentPlane{cpc.p2, normal});
            cpc.p1 = cpc.p2 + cpc.p2 - cpc.p1;
            cpc.p0 = cpc.p2;
            cpc.p2 = cpc.p2 + segment_scale * random_cosine(normal);

            // increment the idx at which the compute shader starts to compute new vertices for the corresponding indices by the number of indices in one segment
            // increment the idx at which the rendering starts by the same amount
            cpc.segment_idx++;
            cpc.indices_start_idx += indices_per_segment;
            tunnel_render_index_start += indices_per_segment;
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
            Buffer& buffer = storage.get_buffer_by_name("firefly_vertices_" + std::to_string(di.current_frame));
            vk::BufferMemoryBarrier buffer_memory_barrier(vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eMemoryWrite, vmc.queue_family_indices.compute, vmc.queue_family_indices.compute, buffer.get(), 0, buffer.get_byte_size());
            cb.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlagBits::eDeviceGroup, {}, {buffer_memory_barrier}, {});
            compute_new_segment(cb, di.current_frame);
            // write copy of data to the first half of the buffer if idx is in the past half of the data
            if (cpc.indices_start_idx > index_count)
            {
                cpc.indices_start_idx -= (index_count + indices_per_segment);
                compute_new_segment(cb, di.current_frame);
                cpc.indices_start_idx += (index_count + indices_per_segment);
            }
            timer.stop(cb, DeviceTimer::COMPUTE_TUNNEL_ADVANCE, vk::PipelineStageFlagBits::eAllCommands);
        }
        cb.end();
    }

    bool TunnelObjects::is_pos_past_segment(glm::vec3 pos, uint32_t idx, bool use_global_id)
    {
        // calculate tunnel local index by subtracting the id of the first segment
        if (use_global_id) idx = std::max(0, int32_t(idx) - int32_t(cpc.segment_idx - segment_count) - 1);
        return glm::dot(glm::normalize(pos - segment_planes[idx].pos), segment_planes[idx].normal) > 0.0f;
    }
}
