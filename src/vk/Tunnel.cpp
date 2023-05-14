#include "vk/Tunnel.hpp"
#include "vk/TunnelObjects.hpp"

namespace ve
{
    Tunnel::Tunnel(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : render_dsh(vmc), vmc(vmc), vcc(vcc), storage(storage), pipeline(vmc), mesh_view_pipeline(vmc)
    {
    }

    void Tunnel::self_destruct(bool full)
    {
        pipeline.self_destruct();
        mesh_view_pipeline.self_destruct();
        if (full)
        {
            render_dsh.self_destruct();
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

        // add one uniform buffer and descriptor set for each frame as the uniform buffer is changed in every frame
        for (uint32_t i = 0; i < parallel_units; ++i)
        {
            model_render_data_buffers.push_back(storage.add_buffer(std::vector<ModelRenderData>{mrd}, vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));

            render_dsh.apply_descriptor_to_new_sets(0, storage.get_buffer(model_render_data_buffers.back()));
            render_dsh.apply_descriptor_to_new_sets(4, storage.get_buffer_by_name("spaceship_lights"));
            render_dsh.new_set();
            render_dsh.reset_auto_apply_bindings();

        }
        render_dsh.construct();

        construct_pipelines(render_pass);
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

    }

    void Tunnel::reload_shaders(const RenderPass& render_pass)
    {
        self_destruct(false);
        construct_pipelines(render_pass);
    }

    void Tunnel::draw(vk::CommandBuffer& cb, DrawInfo& di, uint32_t render_index_start)
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
} // namespace ve
