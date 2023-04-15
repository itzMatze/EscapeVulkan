#include "vk/Scene.hpp"

#include <fstream>

#include "json.hpp"

namespace ve
{
    Scene::Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc) : vmc(vmc), vcc(vcc)
    {}

    void Scene::construct(const RenderPass& render_pass)
    {
        ros.at(ShaderFlavor::Default).construct(render_pass, {std::make_pair("default.vert", vk::ShaderStageFlagBits::eVertex), std::make_pair("default.frag", vk::ShaderStageFlagBits::eFragment)}, vk::PolygonMode::eFill);
        ros.at(ShaderFlavor::Basic).construct(render_pass, {std::make_pair("default.vert", vk::ShaderStageFlagBits::eVertex), std::make_pair("basic.frag", vk::ShaderStageFlagBits::eFragment)}, vk::PolygonMode::eFill);
    }

    void Scene::self_destruct()
    {
        for (auto& ro : ros)
        {
            ro.second.self_destruct();
        }
        ros.clear(); 
        for (auto& model: models)
        {
            model.self_destruct();
        }
        models.clear();
        model_handles.clear();
        loaded = false;
    }

    void Scene::load(const std::string& path)
    {
        ros.emplace(ShaderFlavor::Default, vmc);
        ros.emplace(ShaderFlavor::Basic, vmc);

        ros.at(ShaderFlavor::Default).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        ros.at(ShaderFlavor::Default).dsh.add_binding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);

        ros.at(ShaderFlavor::Basic).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        // load scene from custom json file
        using json = nlohmann::json;
        std::ifstream file(path);
        json data = json::parse(file);
        if (data.contains("model_files"))
        {
            // load referenced model files
            for (const auto& d : data.at("model_files"))
            {
                std::string name = d.value("name", "");
                models.emplace_back(vmc, vcc, std::string("../assets/models/") + std::string(d.value("file", "")));
                model_handles.emplace(name, models.size() - 1);
                if (d.value("ShaderFlavor", "") == "Basic")
                {
                    ros.at(ShaderFlavor::Basic).add_model(models.size() - 1);
                }
                else if (d.value("ShaderFlavor", "") == "Default")
                {
                    ros.at(ShaderFlavor::Default).add_model(models.size() - 1);
                }

                if (d.contains("scale"))
                {
                    glm::vec3 scaling(d.at("scale")[0], d.at("scale")[1], d.at("scale")[2]);
                    scale(name, scaling);
                }
                if (d.contains("translation"))
                {
                    glm::vec3 translation(d.at("translation")[0], d.at("translation")[1], d.at("translation")[2]);
                    translate(name, translation);
                }
                if (d.contains("rotation"))
                {
                    glm::vec3 rotation(d.at("rotation")[1], d.at("rotation")[2], d.at("rotation")[3]);
                    rotate(name, d.at("rotation")[0], rotation);
                }
            }
        }
        // load custom models (vertices and indices directly contained in json file)
        if (data.contains("custom_models"))
        {
            for (const auto& d : data["custom_models"])
            {
                std::string name = d.value("name", "");
                models.emplace_back(vmc, vcc, d);
                model_handles.emplace(name, models.size() - 1);
                if (d.value("ShaderFlavor", "") == "Basic")
                {
                    ros.at(ShaderFlavor::Basic).add_model(models.size() - 1);
                }
                else if (d.value("ShaderFlavor", "") == "Default")
                {
                    ros.at(ShaderFlavor::Default).add_model(models.size() - 1);
                }
            }
        }
        loaded = true;
    }

    void Scene::add_bindings()
    {
        for (auto& ro : ros)
        {
            ro.second.add_bindings(models);
        }
    }

    void Scene::translate(const std::string& model, const glm::vec3& trans)
    {
        if (model_handles.contains(model))
        {
            models[model_handles.at(model)].translate(trans);
        }
        else
        {
            spdlog::warn("Applying translation to not existing model!");
        }
    }

    void Scene::scale(const std::string& model, const glm::vec3& scale)
    {
        if (model_handles.contains(model))
        {
            models[model_handles.at(model)].scale(scale);
        }
        else
        {
            spdlog::warn("Applying scale to not existing model!");
        }
    }

    void Scene::rotate(const std::string& model, float degree, const glm::vec3& axis)
    {
        if (model_handles.contains(model))
        {
            models[model_handles.at(model)].rotate(degree, axis);
        }
        else
        {
            spdlog::warn("Applying rotation to not existing model!");
        }
    }

    DescriptorSetHandler& Scene::get_dsh(ShaderFlavor flavor)
    {
        return ros.at(flavor).dsh;
    }

    void Scene::draw(vk::CommandBuffer& cb, uint32_t current_frame, const glm::mat4& vp)
    {
        for (auto& ro: ros)
        {
            ro.second.draw(cb, current_frame, vp, models);
        }
    }

}// namespace ve
