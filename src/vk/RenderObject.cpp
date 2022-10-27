#include "vk/RenderObject.hpp"

namespace ve
{
    RenderObject::RenderObject(const VulkanMainContext& vmc) : dsh(vmc), vmc(vmc), pipeline(vmc)
    {}

    void RenderObject::self_destruct()
    {
        for (auto& scene: scenes)
        {
            scene.self_destruct();
        }
        scenes.clear();
        pipeline.self_destruct();
        dsh.self_destruct();
    }

    void RenderObject::add_scene(VulkanCommandContext& vcc, const std::string& path, const glm::mat4& transformation)
    {
        scenes.emplace_back(Scene(vmc, vcc, path, transformation));
    }

    void RenderObject::add_bindings()
    {
        for (auto& scene: scenes)
        {
            scene.add_set_bindings(dsh);
        }
    }

    void RenderObject::construct(const vk::RenderPass render_pass, const std::vector<std::pair<std::string, vk::ShaderStageFlagBits>>& shader_names, vk::PolygonMode polygon_mode)
    {
        if (scenes.empty()) return;
        dsh.construct();
        pipeline.construct(render_pass, dsh.get_layouts()[0], shader_names, polygon_mode);
    }

    void RenderObject::draw(vk::CommandBuffer& cb, uint32_t current_frame, const glm::mat4& vp)
    {
        if (scenes.empty()) return;
        cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
        for (auto& scene: scenes)
        {
            scene.draw(current_frame, pipeline.get_layout(), dsh.get_sets(), vp);
        }
    }
}// namespace ve
