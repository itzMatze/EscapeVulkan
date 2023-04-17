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

    struct DrawInfo {
        std::vector<const char*> scene_names;
        int32_t current_scene = 0;
        bool load_scene = false;
        float time_diff = 0.000001f;
		float frametime = 0.0f;
        bool show_ui = true;
        bool mesh_view = false;
        uint32_t current_frame = 0;
        glm::mat4 vp = glm::mat4(1.0);
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
        std::optional<uint32_t> base_texture = std::nullopt;
        std::optional<uint32_t> metallic_roughness_texture = std::nullopt;
        std::optional<uint32_t> normal_texture = std::nullopt;
        std::optional<uint32_t> occlusion_texture = std::nullopt;
        std::optional<uint32_t> emissive_texture = std::nullopt;
    };
} // namespace ve