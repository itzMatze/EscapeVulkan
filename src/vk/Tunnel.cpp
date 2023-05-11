#include "vk/Tunnel.hpp"

namespace ve
{
    constexpr float segment_scale = 50.0f;
    constexpr uint32_t segment_count = 16; // how many segments are in the tunnel (must be power of two)
    static_assert((segment_count & (segment_count - 1)) == 0);
    constexpr uint32_t samples_per_segment = 32; // how many sample rings one segment is made of
    constexpr uint32_t vertices_per_sample = 360; // how many vertices are sampled in one sample ring
    constexpr uint32_t vertex_count = segment_count * samples_per_segment * vertices_per_sample;
    // two triangles per vertex on a sample (3 indices per triangle); every sample of a segment except the last one has triangles
    constexpr uint32_t indices_per_segment = (samples_per_segment - 1) * vertices_per_sample * 6;
    constexpr uint32_t index_count = indices_per_segment * segment_count;

    Tunnel::Tunnel(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : render_dsh(vmc), compute_dsh(vmc), vmc(vmc), vcc(vcc), storage(storage), pipeline(vmc), mesh_view_pipeline(vmc), compute_pipeline(vmc), segment_planes(segment_count), rnd(0), dis(0.0f, 1.0f)
    {
        cpc.indices_start_idx = 0;
    }

    void Tunnel::self_destruct(bool full)
    {
        pipeline.self_destruct();
        mesh_view_pipeline.self_destruct();
        compute_pipeline.self_destruct();
        if (full)
        {
            render_dsh.self_destruct();
            compute_dsh.self_destruct();
            storage.destroy_buffer(vertex_buffer);
            storage.destroy_buffer(index_buffer);
            for (auto i : model_render_data_buffers) storage.destroy_buffer(i);
            model_render_data_buffers.clear();
        }
    }

    void Tunnel::construct(const RenderPass& render_pass, uint32_t parallel_units)
    {
        // double space is needed to enable that new vertices can replace old ones as the tunnel continuously moves forward
        std::vector<Vertex> vertices(vertex_count * 2);
        std::vector<uint32_t> indices(index_count * 2);
        // write indices in advance even for the currently unused space that is reserved for the FixVector behavior
        for (uint32_t i = 0; i < segment_count * 2; ++i)
        {
            for (uint32_t j = 0; j < samples_per_segment - 1; ++j)
            {
                for (uint32_t k = 0; k < vertices_per_sample - 1; ++k)
                {
                    // iterate over all vertices and add 2 triangles per vertex to build the quad that lies in direction of the circle and to the next sample circle
                    const uint32_t indices_idx = i * indices_per_segment + j * vertices_per_sample * 6 + k * 6;
                    const uint32_t vertices_idx = i * samples_per_segment * vertices_per_sample + j * vertices_per_sample + k;
                    indices[indices_idx] = vertices_idx;
                    indices[indices_idx + 1] = vertices_idx + 1;
                    indices[indices_idx + 2] = vertices_idx + vertices_per_sample;
                    indices[indices_idx + 3] = vertices_idx + 1;
                    indices[indices_idx + 4] = vertices_idx + vertices_per_sample + 1;
                    indices[indices_idx + 5] = vertices_idx + vertices_per_sample;
                }
                // close the ring with the last element
                const uint32_t last_indices_idx = i * indices_per_segment + j * vertices_per_sample * 6 + (vertices_per_sample - 1) * 6;
                const uint32_t last_vertices_idx = i * samples_per_segment * vertices_per_sample + j * vertices_per_sample + (vertices_per_sample - 1);
                indices[last_indices_idx] = last_vertices_idx;
                indices[last_indices_idx + 1] = last_vertices_idx + 1 - vertices_per_sample;
                indices[last_indices_idx + 2] = last_vertices_idx + vertices_per_sample;
                indices[last_indices_idx + 3] = last_vertices_idx + 1 - vertices_per_sample;
                indices[last_indices_idx + 4] = last_vertices_idx + 1;
                indices[last_indices_idx + 5] = last_vertices_idx + vertices_per_sample;
            }
        }
        vertex_buffer = storage.add_named_buffer(std::string("tunnel_vertices"), vertices, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);
        index_buffer = storage.add_named_buffer(std::string("tunnel_indices"), indices, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics);

        render_dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        render_dsh.add_binding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);
        compute_dsh.add_binding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
        compute_dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);

        // add one uniform buffer and descriptor set for each frame as the uniform buffer is changed in every frame
        for (uint32_t i = 0; i < parallel_units; ++i)
        {
            model_render_data_buffers.push_back(storage.add_buffer(std::vector<ModelRenderData>{mrd}, vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));

            render_dsh.apply_descriptor_to_new_sets(0, storage.get_buffer(model_render_data_buffers.back()));
            render_dsh.apply_descriptor_to_new_sets(4, storage.get_buffer_by_name("spaceship_lights"));
            render_dsh.new_set();
            render_dsh.reset_auto_apply_bindings();

            compute_dsh.apply_descriptor_to_new_sets(0, storage.get_buffer(index_buffer));
            compute_dsh.apply_descriptor_to_new_sets(1, storage.get_buffer(vertex_buffer));
            compute_dsh.new_set();
            compute_dsh.reset_auto_apply_bindings();
        }
        render_dsh.construct();
        compute_dsh.construct();

        construct_pipelines(render_pass);

        vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[0]);
        cpc.segment_idx = 0;
        cpc.p0 = glm::vec3(0.0f, 0.0f, -50.0f);
        cpc.p1 = glm::vec3(0.0f, 0.0f, -50.0f - segment_scale / 2.0f);
        cpc.p2 = glm::vec3(0.0f, 0.0f, -50.0f - segment_scale);
        segment_planes.push_back(SegmentPlane{cpc.p0, glm::normalize(cpc.p1 - cpc.p0)});
        compute(cb, 0);

        for (uint32_t i = 1; i < segment_count; ++i)
        {
            cpc.segment_idx++;
            cpc.indices_start_idx = i * index_count / segment_count;
            const glm::vec3 normal = glm::normalize(cpc.p2 - cpc.p1);
            segment_planes.push_back(SegmentPlane{cpc.p2, normal});
            cpc.p1 = cpc.p2 + cpc.p2 - cpc.p1;
            cpc.p0 = cpc.p2;
            cpc.p2 = cpc.p0 + segment_scale * random_cosine(normal);
            compute(cb, 0);
        }
        vcc.submit_compute(cb, true);
    }

    void Tunnel::construct_pipelines(const RenderPass& render_pass)
    {
        vk::SpecializationMapEntry uniform_buffer_size_entry(0, 0, sizeof(uint32_t));
        uint32_t uniform_buffer_size = 1;
        vk::SpecializationInfo render_spec_info(1, &uniform_buffer_size_entry, sizeof(uint32_t), &uniform_buffer_size);
        std::vector<ShaderInfo> shader_infos(2);
        shader_infos[0] = ShaderInfo{"default.vert", vk::ShaderStageFlagBits::eVertex, render_spec_info};

        vk::SpecializationMapEntry lights_buffer_size_entry(0, 0, sizeof(uint32_t));
        uint32_t lights_buffer_size = storage.get_buffer_by_name("spaceship_lights").get_element_count();
        vk::SpecializationInfo lights_spec_info(1, &lights_buffer_size_entry, sizeof(uint32_t), &lights_buffer_size);
        shader_infos[1] = ShaderInfo{"stone_noise.frag", vk::ShaderStageFlagBits::eFragment, lights_spec_info};

        pipeline.construct(render_pass, render_dsh.get_layouts()[0], shader_infos, vk::PolygonMode::eFill);
        mesh_view_pipeline.construct(render_pass, render_dsh.get_layouts()[0], shader_infos, vk::PolygonMode::eLine);

        std::array<vk::SpecializationMapEntry, 2> compute_entries;
        compute_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        compute_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
        std::array<uint32_t, 2> compute_entries_data{samples_per_segment, vertices_per_sample};
        vk::SpecializationInfo compute_spec_info(compute_entries.size(), compute_entries.data(), compute_entries_data.size() * sizeof(uint32_t), compute_entries_data.data());

        compute_pipeline.construct(compute_dsh.get_layouts()[0], ShaderInfo{"tunnel.comp", vk::ShaderStageFlagBits::eCompute, compute_spec_info});
    }

    void Tunnel::reload_shaders(const RenderPass& render_pass)
    {
        self_destruct(false);
        construct_pipelines(render_pass);
    }

    void Tunnel::draw(vk::CommandBuffer& cb, DrawInfo& di)
    {
        cb.bindVertexBuffers(0, storage.get_buffer(vertex_buffer).get(), {0});
        cb.bindIndexBuffer(storage.get_buffer(index_buffer).get(), 0, vk::IndexType::eUint32);
        mrd.MVP = di.cam.getVP();
        storage.get_buffer(model_render_data_buffers[di.current_frame]).update_data(std::vector<ModelRenderData>{mrd});
        const vk::PipelineLayout& pipeline_layout = di.mesh_view ? mesh_view_pipeline.get_layout() : pipeline.get_layout();
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, di.mesh_view ? mesh_view_pipeline.get() : pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, render_dsh.get_sets()[di.current_frame], {});
        PushConstants pc{.mvp_idx = 0, .mat_idx = -1, .light_count = di.light_count, .time = di.time, .normal_view = di.normal_view, .tex_view = di.tex_view};
        cb.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, PushConstants::get_vertex_push_constant_size(), &pc);
        cb.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, PushConstants::get_fragment_push_constant_offset(), PushConstants::get_fragment_push_constant_size(), pc.get_fragment_push_constant_pointer());
        cb.drawIndexed(index_count, 1, render_index_start, 0, 0);
    }

    void Tunnel::compute(vk::CommandBuffer& cb, uint32_t current_frame)
    {
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, compute_pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute_pipeline.get_layout(), 0, compute_dsh.get_sets()[current_frame], {});
        cb.pushConstants(compute_pipeline.get_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(ComputePushConstants), &cpc);
        cb.dispatch((vertices_per_sample * samples_per_segment + 31) / 32, 1, 1);
    }

    glm::vec3 Tunnel::random_cosine(const glm::vec3& normal)
    {
        constexpr float cosine_weight = 20.0f;
        float theta = std::acos(std::pow(1.0f - std::abs(dis(rnd)), 1.0f / (1.0f + cosine_weight)));
        float phi = 2.0f * M_PIf * dis(rnd);
        glm::vec3 up = abs(normal.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
        glm::vec3 bitangent = glm::cross(normal, tangent);

        glm::vec3 sample = glm::vec3(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta));
        return glm::normalize(tangent * sample.x + bitangent * sample.y + normal * sample.z);
    }

    bool Tunnel::advance(const DrawInfo& di, DeviceTimer& timer)
    {
        if (glm::dot(glm::normalize(di.player_pos - segment_planes[2].pos), segment_planes[2].normal) > 0.0f)
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
            render_index_start += indices_per_segment;
            // reset indices; compute shader inserts data at the last segment of the region that will be rendered now
            // indices need to be resetted if compute shader would write outside of the buffer or if the render region goes beyond the buffer
            // these 2 conditions are always met at the same time (as compute shader writes last rendered segment)
            if (cpc.indices_start_idx >= index_count * 2)
            {
                cpc.indices_start_idx = index_count - indices_per_segment;
                render_index_start = 0;
            }

            vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[di.current_frame]);
            timer.reset(cb, {DeviceTimer::COMPUTE_TUNNEL_ADVANCE});
            timer.start(cb, DeviceTimer::COMPUTE_TUNNEL_ADVANCE, vk::PipelineStageFlagBits::eAllCommands);
            compute(cb, di.current_frame);
            // write copy of data to the first half of the buffer if idx is in the past half of the data
            if (cpc.indices_start_idx > index_count)
            {
                cpc.indices_start_idx -= (index_count + indices_per_segment);
                compute(cb, di.current_frame);
                cpc.indices_start_idx += (index_count + indices_per_segment);
            }
            timer.stop(cb, DeviceTimer::COMPUTE_TUNNEL_ADVANCE, vk::PipelineStageFlagBits::eAllCommands);
            cb.end();
            return true;
        }
        return false;
    }
} // namespace ve
