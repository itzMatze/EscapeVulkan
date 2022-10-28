#include "vk/Scene.hpp"

#include <fstream>

#include "json.hpp"

namespace ve
{
    Scene::Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc) : vcc(vcc)
    {
        ros.emplace(ShaderFlavor::Default, vmc);
        ros.emplace(ShaderFlavor::Basic, vmc);

        ros.at(ShaderFlavor::Default).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        ros.at(ShaderFlavor::Default).dsh.add_binding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);

        ros.at(ShaderFlavor::Basic).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
    }

    void Scene::construct(const vk::RenderPass& render_pass)
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
        using json = nlohmann::json;
        std::ifstream file(path);
        json data = json::parse(file);
        for (auto& d: data)
        {
            ShaderFlavor flavor;
            if (d.value("ShaderFlavor", "") == "Basic") flavor = ShaderFlavor::Basic;
            if (d.value("ShaderFlavor", "") == "Default") flavor = ShaderFlavor::Default;
            add_model(d.value("name", ""), ModelHandle(flavor, std::string("../assets/models/") + std::string(d.value("file", ""))));

            if (d.contains("scale"))
            {
                glm::vec3 scale(d["scale"][0], d["scale"][1], d["scale"][2]);
                get_model(d.value("name", ""))->scale(scale);
            }
            if (d.contains("translation"))
            {
                glm::vec3 translation(d["translation"][0], d["translation"][1], d["translation"][2]);
                get_model(d.value("name", ""))->translate(translation);
            }
            if (d.contains("rotation"))
            {
                glm::vec3 rotation(d["rotation"][1], d["rotation"][2], d["rotation"][3]);
                get_model(d.value("name", ""))->rotate(d["rotation"][0], rotation);
            }
        }
    }

    void Scene::add_model(const std::string& key, ModelHandle model_handle)
    {
        if (model_handle.filename != "none")
        {
            model_handle.idx = ros.at(model_handle.shader_flavor).add_model(vcc, model_handle.filename);
        }
        else
        {
            model_handle.idx = ros.at(model_handle.shader_flavor).add_model(vcc, *model_handle.vertices, *model_handle.indices, model_handle.material);
        }
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
