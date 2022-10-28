#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

namespace ve
{
    enum class ShaderFlavor
    {
        Basic,
        Default
    };

    struct PushConstants {
        glm::mat4 MVP;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec4 color;
        glm::vec2 tex;

        static vk::VertexInputBindingDescription get_binding_description()
        {
            vk::VertexInputBindingDescription binding_description{};
            binding_description.binding = 0;
            binding_description.stride = sizeof(Vertex);
            binding_description.inputRate = vk::VertexInputRate::eVertex;
            return binding_description;
        }

        static std::array<vk::VertexInputAttributeDescription, 4> get_attribute_descriptions()
        {
            std::array<vk::VertexInputAttributeDescription, 4> attribute_descriptions{};
            attribute_descriptions[0].binding = 0;
            attribute_descriptions[0].location = 0;
            attribute_descriptions[0].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[0].offset = offsetof(Vertex, pos);

            attribute_descriptions[1].binding = 0;
            attribute_descriptions[1].location = 1;
            attribute_descriptions[1].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[1].offset = offsetof(Vertex, normal);

            attribute_descriptions[2].binding = 0;
            attribute_descriptions[2].location = 2;
            attribute_descriptions[2].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[2].offset = offsetof(Vertex, color);

            attribute_descriptions[3].binding = 0;
            attribute_descriptions[3].location = 3;
            attribute_descriptions[3].format = vk::Format::eR32G32Sfloat;
            attribute_descriptions[3].offset = offsetof(Vertex, tex);

            return attribute_descriptions;
        }
    };

    class Image;

    struct Material {
        float metallic = 1.0f;
        float roughness = 1.0f;
        glm::vec4 base_color = glm::vec4(1.0f);
        glm::vec4 emission = glm::vec4(1.0f);
        Image* base_texture;
        Image* metallic_roughness_texture;
        Image* normal_texture;
        Image* occlusion_texture;
        Image* emissive_texture;
    };

    struct ModelHandle {
        ModelHandle(ShaderFlavor flavor, const std::string& file) : shader_flavor(flavor), idx(0), filename(file)
        {}
        ModelHandle(ShaderFlavor flavor, const std::vector<Vertex>* vertices, const std::vector<uint32_t>* indices, const Material* material) : shader_flavor(flavor), idx(0), filename("none"), vertices(vertices), indices(indices), material(material)
        {}
        ShaderFlavor shader_flavor;
        uint32_t idx;
        std::string filename;
        const std::vector<Vertex>* vertices;
        const std::vector<uint32_t>* indices;
        const Material* material;
    };
}// namespace ve