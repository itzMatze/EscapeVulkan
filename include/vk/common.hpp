#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "VulkanWithDefines.hpp"

class Camera;

namespace ve
{
    constexpr uint32_t frames_in_flight = 2;
    constexpr uint32_t distance_directions_count = 5;

    enum class ShaderFlavor
    {
        Basic = 0,
        Default = 1,
        Emissive = 2,
        Size = 3
    };

    struct MeshRenderData {
        int32_t model_render_data_idx;
        int32_t mat_idx;
        uint32_t indices_idx;
    };

    struct ModelRenderData {
        glm::mat4 MVP = glm::mat4(1.0f);
        glm::mat4 prev_MVP = glm::mat4(1.0f);
        glm::mat4 M = glm::mat4(1.0f);
        alignas(16) int32_t segment_uid;
    };

    struct RenderPushConstants {
        uint32_t mesh_render_data_idx;
    };

    struct DebugPushConstants {
        glm::mat4 mvp;
    };

    struct SessionData
    {
        std::vector<float> devicetimings;
        int32_t current_scene = 0;
    };

    struct CollisionResults
    {
        int32_t collision_detected = 0;
        std::array<float, distance_directions_count> distances;
    };

    struct PlayerData
    {
        glm::vec4 pos = glm::vec4(0.0f);
        glm::vec4 dir = glm::vec4(0.0f);
        glm::vec4 up = glm::vec4(0.0f);
        uint32_t segment_id = 0;
    };

    struct FrameData
    {
        glm::vec4 player_pos = glm::vec4(0.0f);
        glm::vec4 player_dir = glm::vec4(0.0f);
        glm::vec4 player_up = glm::vec4(0.0f);
        uint32_t player_segment_id = 0;
        float time_diff = 0.000001f;
        float time = 0.0f;
        uint32_t tunnel_first_segment_indices_idx = 0;
        uint32_t color_view = false;
        uint32_t normal_view = false;
        uint32_t tex_view = false;
        uint32_t segment_uid_view = false;
    };

    struct GameData
    {
        PlayerData player_data;
        CollisionResults collision_results;
        float time_diff = 0.000001f;
        float time = 0.0f;
        float frametime = 0.0f;
        float player_reset_blink_timer = 0.0f;
        uint32_t player_reset_blink_counter = 0;
        uint32_t player_lifes = 3;
        float segment_distance_travelled = 0.0f;
        float tunnel_distance_travelled = 0.0f;
        uint32_t current_frame = 0;
        uint32_t total_frames = 0;
        uint32_t first_segment_indices_idx = 0;
        bool show_player = true;
    };

    struct Settings
    {
        bool show_ui = true;
        bool mesh_view = false;
        bool normal_view = false;
        bool color_view = false;
        bool segment_uid_view = false;
        bool tex_view = false;
        bool show_player_bb = false;
        bool collision_detection_active = true;
        bool save_screenshot = false;
        bool disable_rendering = false;
    };

    struct GameState
    {
        SessionData session_data;
        GameData game_data;
        Settings settings;
        Camera& cam;
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

    struct Light {
        glm::vec3 dir;
        float intensity = 1.0f;
        glm::vec3 pos;
        float innerConeAngle;
        glm::vec3 color;
        float outerConeAngle;
    };

    struct ModelMatrices {
        glm::mat4 m;
        glm::mat4 inv_m;
    };

    struct Reservoir {
        uint32_t y = 0;
        float w = 0.0f;
        uint32_t M = 1;
        float W = 0.0f;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec4 color;
        glm::vec2 tex;

        static std::vector<vk::VertexInputBindingDescription> get_binding_descriptions()
        {
            vk::VertexInputBindingDescription binding_description{};
            binding_description.binding = 0;
            binding_description.stride = sizeof(Vertex);
            binding_description.inputRate = vk::VertexInputRate::eVertex;
            return {binding_description};
        }

        static std::vector<vk::VertexInputAttributeDescription> get_attribute_descriptions()
        {
            std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(4);
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

    struct DebugVertex {
        glm::vec3 pos;
        glm::vec4 color;

        static std::vector<vk::VertexInputBindingDescription> get_binding_descriptions()
        {
            vk::VertexInputBindingDescription binding_description{};
            binding_description.binding = 0;
            binding_description.stride = sizeof(DebugVertex);
            binding_description.inputRate = vk::VertexInputRate::eVertex;
            return {binding_description};
        }

        static std::vector<vk::VertexInputAttributeDescription> get_attribute_descriptions()
        {
            std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(2);
            attribute_descriptions[0].binding = 0;
            attribute_descriptions[0].location = 0;
            attribute_descriptions[0].format = vk::Format::eR32G32B32Sfloat;
            attribute_descriptions[0].offset = offsetof(DebugVertex, pos);

            attribute_descriptions[1].binding = 0;
            attribute_descriptions[1].location = 1;
            attribute_descriptions[1].format = vk::Format::eR32G32B32A32Sfloat;
            attribute_descriptions[1].offset = offsetof(DebugVertex, color);

            return attribute_descriptions;
        }
    };
} // namespace ve

