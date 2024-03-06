#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <cstdint>

#include "vk/VulkanWithDefines.hpp"

namespace ve
{
    struct TunnelSkyboxPushConstants
    {
        glm::mat4 mvp;
    };

    struct TunnelVertex {
        glm::vec3 pos;
        glm::vec2 normal;
        glm::vec2 tex;
        uint32_t segment_uid;

        static std::vector<vk::VertexInputBindingDescription> get_binding_descriptions()
        {
            vk::VertexInputBindingDescription binding_description{};
            binding_description.binding = 0;
            binding_description.stride = sizeof(TunnelVertex);
            binding_description.inputRate = vk::VertexInputRate::eVertex;
            return {binding_description};
        }

        static std::vector<vk::VertexInputAttributeDescription> get_attribute_descriptions()
        {
            std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(4);
            attribute_descriptions[0].binding = 0;
            attribute_descriptions[0].location = 0;
            attribute_descriptions[0].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[0].offset = offsetof(TunnelVertex, pos);

            attribute_descriptions[1].binding = 0;
            attribute_descriptions[1].location = 1;
            attribute_descriptions[1].format = vk::Format::eR32G32Sfloat;
            attribute_descriptions[1].offset = offsetof(TunnelVertex, normal);

            attribute_descriptions[2].binding = 0;
            attribute_descriptions[2].location = 2;
            attribute_descriptions[2].format = vk::Format::eR32G32Sfloat;
            attribute_descriptions[2].offset = offsetof(TunnelVertex, tex);

            attribute_descriptions[3].binding = 0;
            attribute_descriptions[3].location = 3;
            attribute_descriptions[3].format = vk::Format::eR32Sfloat;
            attribute_descriptions[3].offset = offsetof(TunnelVertex, segment_uid);

            return attribute_descriptions;
        }
    };

    struct TunnelSkyboxVertex
    {
        glm::vec3 pos;
        glm::vec2 tex;

        static std::vector<vk::VertexInputBindingDescription> get_binding_descriptions()
        {
            vk::VertexInputBindingDescription binding_description{};
            binding_description.binding = 0;
            binding_description.stride = sizeof(TunnelSkyboxVertex);
            binding_description.inputRate = vk::VertexInputRate::eVertex;
            return {binding_description};
        }

        static std::vector<vk::VertexInputAttributeDescription> get_attribute_descriptions()
        {
            std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(2);
            attribute_descriptions[0].binding = 0;
            attribute_descriptions[0].location = 0;
            attribute_descriptions[0].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[0].offset = offsetof(TunnelSkyboxVertex, pos);

            attribute_descriptions[1].binding = 0;
            attribute_descriptions[1].location = 1;
            attribute_descriptions[1].format = vk::Format::eR32G32Sfloat;
            attribute_descriptions[1].offset = offsetof(TunnelSkyboxVertex, tex);

            return attribute_descriptions;
        }
    };
} // namespace ve

