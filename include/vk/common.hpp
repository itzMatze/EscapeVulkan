#pragma once

#include <optional>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

namespace ve
{
    enum class ShaderFlavor
    {
        Basic = 0,
        Default = 1,
        Emissive = 2,
        Size = 3
    };

    struct ModelRenderData {
        glm::mat4 MVP;
    };

    struct PushConstants {
        // vertex push constants
        uint32_t mvp_idx;
        // fragment push constants
        int32_t mat_idx;
        bool normal_view;

        void* get_fragment_push_constant_pointer()
        { return &mat_idx; }

        static uint32_t get_vertex_push_constant_size()
        { return offsetof(PushConstants, mat_idx); }

        static uint32_t get_vertex_push_constant_offset()
        { return 0; }

        static uint32_t get_fragment_push_constant_size()
        { return sizeof(PushConstants) - offsetof(PushConstants, mat_idx); }

        static uint32_t get_fragment_push_constant_offset()
        { return offsetof(PushConstants, mat_idx); }
    };

    struct DrawInfo {
        std::vector<const char*> scene_names;
        int32_t current_scene = 0;
        bool load_scene = false;
        float time_diff = 0.000001f;
		float frametime = 0.0f;
        std::vector<float> devicetimings;
        bool show_ui = true;
        bool mesh_view = false;
        bool normal_view = false;
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

    struct Material {
        glm::vec4 base_color = glm::vec4(1.0f);
        glm::vec4 emission = glm::vec4(0.0f);
        float metallic = 0.0f;
        float roughness = 0.0f;
        alignas(8) int32_t base_texture = -1;
        //int32_t metallic_roughness_texture = -1;
        //int32_t normal_texture = -1;
        //int32_t occlusion_texture = -1;
        //int32_t emissive_texture = -1;
    };
} // namespace ve
