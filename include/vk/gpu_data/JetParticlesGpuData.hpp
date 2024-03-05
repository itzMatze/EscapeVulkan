#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <cstdint>

#include "vk/VulkanWithDefines.hpp"

namespace ve
{
    struct JetParticleMovePushConstants
    {
        alignas(16) glm::vec3 move_dir;
        float time;
        float time_diff;
    };

    struct JetParticleVertex
    {
        glm::vec3 pos;
        glm::vec3 col;
        glm::vec3 vel;
        float lifetime;
        uint64_t pad;

        static std::vector<vk::VertexInputBindingDescription> get_binding_descriptions()
        {
            vk::VertexInputBindingDescription binding_description{};
            binding_description.binding = 0;
            binding_description.stride = sizeof(JetParticleVertex);
            binding_description.inputRate = vk::VertexInputRate::eVertex;
            return {binding_description};
        }

        static std::vector<vk::VertexInputAttributeDescription> get_attribute_descriptions()
        {
            std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(3);
            attribute_descriptions[0].binding = 0;
            attribute_descriptions[0].location = 0;
            attribute_descriptions[0].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[0].offset = offsetof(JetParticleVertex, pos);

            attribute_descriptions[1].binding = 0;
            attribute_descriptions[1].location = 1;
            attribute_descriptions[1].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[1].offset = offsetof(JetParticleVertex, col);

            // attribute_descriptions[2].binding = 0;
            // attribute_descriptions[2].location = 2;
            // attribute_descriptions[2].format = vk::Format::eR32G32B32Sfloat;
            // attribute_descriptions[2].offset = offsetof(JetParticleVertex, vel);

            attribute_descriptions[2].binding = 0;
            attribute_descriptions[2].location = 3;
            attribute_descriptions[2].format = vk::Format::eR32Sfloat;
            attribute_descriptions[2].offset = offsetof(JetParticleVertex, lifetime);

            return attribute_descriptions;
        }
    };
} // namespace ve

