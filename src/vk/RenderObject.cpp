#include "vk/RenderObject.hpp"

namespace ve
{
    RenderObject::RenderObject(const VulkanMainContext& vmc) : dsh(vmc), vmc(vmc), pipeline(vmc)
    {}

    void RenderObject::self_destruct()
    {
        for (auto& model: models)
        {
            model.self_destruct();
        }
        models.clear();
        pipeline.self_destruct();
        dsh.self_destruct();
    }

    uint32_t RenderObject::add_model(VulkanCommandContext& vcc, const std::string& path)
    {
        models.emplace_back(Model(vmc, vcc, path));
        return (models.size() - 1);
    }

    uint32_t RenderObject::add_model(VulkanCommandContext& vcc, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const Material* material)
    {
        models.emplace_back(vmc, vcc, vertices, indices, material);
        return (models.size() - 1);
    }

    Model* RenderObject::get_model(uint32_t idx)
    {
        return &models[idx];
    }

    void RenderObject::add_bindings()
    {
        for (auto& model: models)
        {
            model.add_set_bindings(dsh);
        }
    }

    void RenderObject::construct(const RenderPass& render_pass, const std::vector<std::pair<std::string, vk::ShaderStageFlagBits>>& shader_names, vk::PolygonMode polygon_mode)
    {
        if (models.empty()) return;
        dsh.construct();
        pipeline.construct(render_pass, dsh.get_layouts()[0], shader_names, polygon_mode);
    }

    void RenderObject::draw(vk::CommandBuffer& cb, uint32_t current_frame, const glm::mat4& vp)
    {
        if (models.empty()) return;
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
        for (auto& model: models)
        {
            model.draw(current_frame, pipeline.get_layout(), dsh.get_sets(), vp);
        }
    }
}// namespace ve
