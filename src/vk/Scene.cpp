#include "vk/Scene.hpp"

namespace ve
{
    Scene::Scene(const VulkanMainContext& vmc)
    {
        ros.emplace(ShaderFlavor::Default, vmc);
        ros.emplace(ShaderFlavor::Basic, vmc);

        ros.at(ShaderFlavor::Default).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        ros.at(ShaderFlavor::Default).dsh.add_binding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);

        ros.at(ShaderFlavor::Basic).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
    }

    void Scene::construct_models(VulkanCommandContext& vcc)
    {
        for (auto& model_handle: model_handles)
        {
            if (model_handle.second.filename != "none")
            {
                model_handle.second.idx = ros.at(model_handle.second.shader_flavor).add_model(vcc, model_handle.second.filename);
            }
            else
            {
                model_handle.second.idx = ros.at(model_handle.second.shader_flavor).add_model(vcc, *model_handle.second.vertices, *model_handle.second.indices, model_handle.second.material);
            }
        }
    }

    void Scene::construct_render_objects(const vk::RenderPass& render_pass)
    {
        ros.at(ShaderFlavor::Default).construct(render_pass, {std::make_pair("default.vert", vk::ShaderStageFlagBits::eVertex), std::make_pair("default.frag", vk::ShaderStageFlagBits::eFragment)}, vk::PolygonMode::eFill);
        ros.at(ShaderFlavor::Basic).construct(render_pass, {std::make_pair("default.vert", vk::ShaderStageFlagBits::eVertex), std::make_pair("basic.frag", vk::ShaderStageFlagBits::eFragment)}, vk::PolygonMode::eFill);
    }

    void Scene::self_destruct()
    {
        for (auto& ro: ros)
        {
            ro.second.self_destruct();
        }
        ros.clear();
    }

    void Scene::load(const std::string& path)
    {
    }

    void Scene::add_model(const std::string& key, ModelHandle model_handle)
    {
        model_handles.emplace(key, model_handle);
    }

    void Scene::add_bindings()
    {
        for (auto& ro: ros)
        {
            ro.second.add_bindings();
        }
    }

    Model* Scene::get_model(const std::string& key)
    {
        if (model_handles.contains(key))
        {
            return ros.at(model_handles.at(key).shader_flavor).get_model(model_handles.at(key).idx);
        }
        return nullptr;
    }

    DescriptorSetHandler& Scene::get_dsh(ShaderFlavor flavor)
    {
        return ros.at(flavor).dsh;
    }

    void Scene::draw(vk::CommandBuffer& cb, uint32_t current_frame, const glm::mat4& vp)
    {
        for (auto& ro: ros)
        {
            ro.second.draw(cb, current_frame, vp);
        }
    }

}// namespace ve
