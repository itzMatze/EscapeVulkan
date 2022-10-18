#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.hpp>

namespace ve
{
    struct PushConstants {
        glm::mat4 MVP;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
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
}// namespace ve