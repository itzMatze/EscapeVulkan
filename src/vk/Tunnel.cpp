#include "vk/Tunnel.hpp"
#include "vk/TunnelObjects.hpp"

namespace ve
{
    Tunnel::Tunnel(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage) : render_dsh(vmc), vmc(vmc), vcc(vcc), storage(storage), pipeline(vmc), mesh_view_pipeline(vmc)
    {}

    void Tunnel::self_destruct(bool full)
    {
        pipeline.self_destruct();
        mesh_view_pipeline.self_destruct();
        render_dsh.self_destruct();
        storage.destroy_image(noise_textures);
        for (auto i : model_render_data_buffers) storage.destroy_buffer(i);
        model_render_data_buffers.clear();
        if (full)
        {
            storage.destroy_buffer(vertex_buffer);
            storage.destroy_buffer(index_buffer);
        }
    }

    void Tunnel::create_buffers()
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
    }

    void Tunnel::construct(const RenderPass& render_pass)
    {
        construct_pipelines(render_pass);
    }

    void Tunnel::construct_pipelines(const RenderPass& render_pass)
    {
        create_noise_textures();

        render_dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        render_dsh.add_binding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        render_dsh.add_binding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);
        render_dsh.add_binding(5, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);

        // add one uniform buffer and descriptor set for each frame as the uniform buffer is changed in every frame
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            model_render_data_buffers.push_back(storage.add_buffer(std::vector<ModelRenderData>{mrd}, vk::BufferUsageFlagBits::eUniformBuffer, false, vmc.queue_family_indices.transfer, vmc.queue_family_indices.graphics));

            render_dsh.new_set();
            render_dsh.add_descriptor(0, storage.get_buffer(model_render_data_buffers.back()));
            render_dsh.add_descriptor(1, storage.get_image(noise_textures));
            render_dsh.add_descriptor(4, storage.get_buffer_by_name("spaceship_lights"));
            render_dsh.add_descriptor(5, storage.get_buffer_by_name("firefly_vertices_" + std::to_string(i)));
        }
        render_dsh.construct();

        vk::SpecializationMapEntry uniform_buffer_size_entry(0, 0, sizeof(uint32_t));
        uint32_t uniform_buffer_size = 1;
        vk::SpecializationInfo render_spec_info(1, &uniform_buffer_size_entry, sizeof(uint32_t), &uniform_buffer_size);
        std::vector<ShaderInfo> shader_infos(2);
        shader_infos[0] = ShaderInfo{"tunnel.vert", vk::ShaderStageFlagBits::eVertex, render_spec_info};

        std::array<vk::SpecializationMapEntry, 3> fragment_entries;
        fragment_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        fragment_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
        fragment_entries[2] = vk::SpecializationMapEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t));
        std::array<uint32_t, 3> fragment_entries_data{uint32_t(storage.get_buffer_by_name("spaceship_lights").get_element_count()), segment_count, fireflies_per_segment};
        vk::SpecializationInfo fragment_spec_info(fragment_entries.size(), fragment_entries.data(), sizeof(uint32_t) * fragment_entries_data.size(), fragment_entries_data.data());
        shader_infos[1] = ShaderInfo{"tunnel.frag", vk::ShaderStageFlagBits::eFragment, fragment_spec_info};

        pipeline.construct(render_pass, render_dsh.get_layouts()[0], shader_infos, vk::PolygonMode::eFill, TunnelVertex::get_binding_descriptions(), TunnelVertex::get_attribute_descriptions());
        mesh_view_pipeline.construct(render_pass, render_dsh.get_layouts()[0], shader_infos, vk::PolygonMode::eLine, TunnelVertex::get_binding_descriptions(), TunnelVertex::get_attribute_descriptions());
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
        PushConstants pc{.mvp_idx = 0, .mat_idx = -1, .time = di.time, .normal_view = di.normal_view, .tex_view = di.tex_view};
        cb.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, PushConstants::get_vertex_push_constant_size(), &pc);
        cb.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, PushConstants::get_fragment_push_constant_offset(), PushConstants::get_fragment_push_constant_size(), pc.get_fragment_push_constant_pointer());
        cb.drawIndexed(index_count, 1, render_index_start, 0, 0);
    }

    void Tunnel::create_noise_textures()
    {
        constexpr uint32_t noise_texture_dim = 2048;
        DescriptorSetHandler pre_process_dsh(vmc);
        pre_process_dsh.add_binding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute);
        std::vector<std::vector<unsigned char>> texture_data;
        for (uint32_t i = 0; i < 2; ++i) texture_data.push_back(std::vector<unsigned char>(noise_texture_dim * noise_texture_dim * 4, 0));
        noise_textures = storage.add_named_image(std::string("noise_textures"), texture_data, noise_texture_dim, noise_texture_dim, false, 0, std::vector<uint32_t>{vmc.queue_family_indices.graphics, vmc.queue_family_indices.transfer, vmc.queue_family_indices.compute}, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage);
        storage.get_image(noise_textures).transition_image_layout(vcc, vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::AccessFlagBits::eNone, vk::AccessFlagBits::eNone);
        pre_process_dsh.new_set();
        pre_process_dsh.add_descriptor(0, storage.get_image(noise_textures));
        pre_process_dsh.construct();

        Pipeline pre_process_pipeline(vmc);
        pre_process_pipeline.construct(pre_process_dsh.get_layouts()[0], ShaderInfo{"create_noise_textures.comp", vk::ShaderStageFlagBits::eCompute}, 0);
        vk::CommandBuffer& cb = vcc.begin(vcc.compute_cb[0]);
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, pre_process_pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pre_process_pipeline.get_layout(), 0, pre_process_dsh.get_sets()[0], {});
        cb.dispatch(noise_texture_dim / 32, noise_texture_dim / 32, 1);
        vcc.submit_compute(cb, true);
        storage.get_image(noise_textures).transition_image_layout(vcc, vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
        pre_process_dsh.self_destruct();
        pre_process_pipeline.self_destruct();
    }
} // namespace ve
