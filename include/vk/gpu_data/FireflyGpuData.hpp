#include <glm/vec3.hpp>
#include <cstdint>

#include "vk/VulkanWithDefines.hpp"

namespace ve
{
    struct FireflyMovePushConstants
    {
        float time;
        float time_diff;
        uint32_t segment_uid;
        uint32_t first_segment_indices_idx;
    };

    struct FireflyVertex
    {
        glm::vec3 pos;
        glm::vec3 col;
        glm::vec3 vel;
        glm::vec3 acc;

        static std::vector<vk::VertexInputBindingDescription> get_binding_descriptions()
        {
            vk::VertexInputBindingDescription binding_description{};
            binding_description.binding = 0;
            binding_description.stride = sizeof(FireflyVertex);
            binding_description.inputRate = vk::VertexInputRate::eVertex;
            return {binding_description};
        }

        static std::vector<vk::VertexInputAttributeDescription> get_attribute_descriptions()
        {
            std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(2);
            attribute_descriptions[0].binding = 0;
            attribute_descriptions[0].location = 0;
            attribute_descriptions[0].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[0].offset = offsetof(FireflyVertex, pos);

            attribute_descriptions[1].binding = 0;
            attribute_descriptions[1].location = 1;
            attribute_descriptions[1].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[1].offset = offsetof(FireflyVertex, col);

            //attribute_descriptions[2].binding = 0;
            //attribute_descriptions[2].location = 2;
            //attribute_descriptions[2].format = vk::Format::eR32G32B32Sfloat;
            //attribute_descriptions[2].offset = offsetof(FireflyVertex, vel);

            //attribute_descriptions[3].binding = 0;
            //attribute_descriptions[3].location = 3;
            //attribute_descriptions[3].format = vk::Format::eR32G32B32Sfloat;
            //attribute_descriptions[3].offset = offsetof(FireflyVertex, acc);

            return attribute_descriptions;
        }
    };
} // namespace ve

