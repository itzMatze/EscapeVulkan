#include "vk/RenderObject.hpp"

namespace ve
{
    RenderObject::RenderObject(const VulkanMainContext& vmc) : dsh(vmc), vmc(vmc), pipeline(vmc)
    {}

    void RenderObject::self_destruct()
    {
        pipeline.self_destruct();
        dsh.self_destruct();
    }

    void RenderObject::add_model(uint32_t idx)
    {
        model_indices.emplace_back(idx);
    }

    void RenderObject::add_bindings(std::vector<Model>& models)
    {
        for (auto& model : model_indices)
        {
            models[model].add_set_bindings(dsh);
        }
    }

    void RenderObject::construct(const RenderPass& render_pass, const std::vector<std::pair<std::string, vk::ShaderStageFlagBits>>& shader_names, vk::PolygonMode polygon_mode)
    {
        if (model_indices.empty()) return;
        dsh.construct();
        pipeline.construct(render_pass, dsh.get_layouts()[0], shader_names, polygon_mode);
    }

    void RenderObject::draw(vk::CommandBuffer& cb, uint32_t current_frame, const glm::mat4& vp, std::vector<Model>& models)
    {
        if (model_indices.empty()) return;
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
        for (auto& model : model_indices)
        {
            models[model].draw(current_frame, pipeline.get_layout(), dsh.get_sets(), vp);
        }
    }
} // namespace ve
