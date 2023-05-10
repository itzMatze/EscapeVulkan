#include "vk/RenderObject.hpp"

namespace ve
{
    RenderObject::RenderObject(const VulkanMainContext& vmc) : dsh(vmc), vmc(vmc), pipeline(vmc), mesh_view_pipeline(vmc)
    {}

    void RenderObject::self_destruct(bool full)
    {
        pipeline.self_destruct();
        mesh_view_pipeline.self_destruct();
        if (full)
        {
            dsh.self_destruct();
        }
    }

    void RenderObject::add_model_meshes(std::vector<Mesh>& mesh_list)
    {
        model_indices.push_back(meshes.size());
        meshes.insert(meshes.end(), mesh_list.begin(), mesh_list.end());
    }

    void RenderObject::add_bindings()
    {
        dsh.new_set();
    }

    void RenderObject::construct(const RenderPass& render_pass, const std::vector<ShaderInfo>& shader_infos, bool reload)
    {
        if (meshes.empty()) return;
        if (!reload)
        {
            model_indices.push_back(meshes.size());
            dsh.construct();
        }
        else
        {
            self_destruct(false);
        }
        construct_pipelines(render_pass, shader_infos);
    }

    void RenderObject::construct_pipelines(const RenderPass& render_pass, const std::vector<ShaderInfo>& shader_infos)
    {
        pipeline.construct(render_pass, dsh.get_layouts()[0], shader_infos, vk::PolygonMode::eFill);
        mesh_view_pipeline.construct(render_pass, dsh.get_layouts()[0], shader_infos, vk::PolygonMode::eLine);
    }

    void RenderObject::draw(vk::CommandBuffer& cb, DrawInfo& di)
    {
        if (meshes.empty()) return;
        const vk::PipelineLayout& pipeline_layout = di.mesh_view ? mesh_view_pipeline.get_layout() : pipeline.get_layout();
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, di.mesh_view ? mesh_view_pipeline.get() : pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, dsh.get_sets()[di.current_frame], {});
        for (uint32_t i = 0; i < model_indices.size() - 1; ++i)
        {
            PushConstants pc{.mvp_idx = i};
            cb.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, PushConstants::get_vertex_push_constant_size(), &pc);
            for (uint32_t j = model_indices[i]; j < model_indices[i + 1]; ++j) meshes[j].draw(cb, pipeline_layout, dsh.get_sets(), di);
        }
    }
} // namespace ve
