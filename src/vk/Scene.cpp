#include "vk/Scene.hpp"

#include <fstream>

#include "json.hpp"

namespace ve
{
    Scene::Scene(const VulkanMainContext& vmc, VulkanCommandContext& vcc) : vmc(vmc), vcc(vcc)
    {
        ros.emplace(ShaderFlavor::Default, vmc);
        ros.emplace(ShaderFlavor::Basic, vmc);

        ros.at(ShaderFlavor::Default).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        ros.at(ShaderFlavor::Default).dsh.add_binding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);

        ros.at(ShaderFlavor::Basic).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
    }

    void Scene::construct(const RenderPass& render_pass)
    {
        ros.at(ShaderFlavor::Default).construct(render_pass, {std::make_pair("default.vert", vk::ShaderStageFlagBits::eVertex), std::make_pair("default.frag", vk::ShaderStageFlagBits::eFragment)}, vk::PolygonMode::eFill);
        ros.at(ShaderFlavor::Basic).construct(render_pass, {std::make_pair("default.vert", vk::ShaderStageFlagBits::eVertex), std::make_pair("basic.frag", vk::ShaderStageFlagBits::eFragment)}, vk::PolygonMode::eFill);
    }

    void Scene::self_destruct()
    {
        for (auto& image: images)
        {
            image.self_destruct();
        }
        images.clear();
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
        if (data.contains("model_files"))
        {
            for (auto& d: data["model_files"])
            {
                ShaderFlavor flavor;
                if (d.value("ShaderFlavor", "") == "Basic") flavor = ShaderFlavor::Basic;
                if (d.value("ShaderFlavor", "") == "Default") flavor = ShaderFlavor::Default;
                std::string name = d.value("name", "");
                add_model(name, ModelHandle(flavor, std::string("../assets/models/") + std::string(d.value("file", ""))));

                if (d.contains("scale"))
                {
                    glm::vec3 scaling(d["scale"][0], d["scale"][1], d["scale"][2]);
                    scale(name, scaling);
                }
                if (d.contains("translation"))
                {
                    glm::vec3 translation(d["translation"][0], d["translation"][1], d["translation"][2]);
                    translate(name, translation);
                }
                if (d.contains("rotation"))
                {
                    glm::vec3 rotation(d["rotation"][1], d["rotation"][2], d["rotation"][3]);
                    rotate(name, d["rotation"][0], rotation);
                }
            }
        }
        if (data.contains("custom_models"))
        {
            for (auto& d: data["custom_models"])
            {
                ShaderFlavor flavor;
                if (d.value("ShaderFlavor", "") == "Basic") flavor = ShaderFlavor::Basic;
                if (d.value("ShaderFlavor", "") == "Default") flavor = ShaderFlavor::Default;
                std::string name = d.value("name", "");
                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices;
                for (auto& v: d["vertices"])
                {
                    Vertex vertex;
                    vertex.pos = glm::vec3(v["pos"][0], v["pos"][1], v["pos"][2]);
                    vertex.normal = glm::vec3(v["normal"][0], v["normal"][1], v["normal"][2]);
                    vertex.color = glm::vec4(v["color"][0], v["color"][1], v["color"][2], v["color"][3]);
                    vertex.tex = glm::vec2(v["tex"][0], v["tex"][1]);
                    vertices.push_back(vertex);
                }
                for (auto& i: d["indices"])
                {
                    indices.push_back(i);
                }
                Material m;
                if (d.contains("base_texture"))
                {
                    images.emplace_back(Image(vmc, vcc, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, std::string("../assets/textures/") + std::string(d.value("base_texture", "")), true));
                    m.base_texture = &images.back();
                }
                materials.push_back(m);
                add_model(name, ModelHandle(flavor, &vertices, &indices, &materials.back()));
            }
        }
    }

    void Scene::add_model(const std::string& key, ModelHandle model_handle)
    {
        if (model_handle.shader_flavor == ShaderFlavor::Basic) model_handle.material = nullptr;
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

    void Scene::translate(const std::string& model, const glm::vec3& trans)
    {
        if (model_handles.contains(model))
        {
            ros.at(model_handles.at(model).shader_flavor).get_model(model_handles.at(model).idx)->translate(trans);
        }
        else
        {
            VE_LOG_CONSOLE(VE_WARN, VE_C_YELLOW << "Applying translation to not existing model!\n");
        }
    }

    void Scene::scale(const std::string& model, const glm::vec3& scale)
    {
        if (model_handles.contains(model))
        {
            ros.at(model_handles.at(model).shader_flavor).get_model(model_handles.at(model).idx)->scale(scale);
        }
        else
        {
            VE_LOG_CONSOLE(VE_WARN, VE_C_YELLOW << "Applying scale to not existing model!\n");
        }
    }

    void Scene::rotate(const std::string& model, float degree, const glm::vec3& axis)
    {
        if (model_handles.contains(model))
        {
            ros.at(model_handles.at(model).shader_flavor).get_model(model_handles.at(model).idx)->rotate(degree, axis);
        }
        else
        {
            VE_LOG_CONSOLE(VE_WARN, VE_C_YELLOW << "Applying rotation to not existing model!\n");
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
            ro.second.draw(cb, current_frame, vp);
        }
    }

}// namespace ve
