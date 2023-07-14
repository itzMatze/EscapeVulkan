#pragma once

#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <vector>

#include "UI.hpp"
#include "vk/common.hpp"
#include "vk/Buffer.hpp"
#include "vk/Scene.hpp"
#include "vk/Swapchain.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk/VulkanMainContext.hpp"
#include "Storage.hpp"
#include "vk/Timer.hpp"

namespace ve
{
    class WorkContext
    {
    public:
        WorkContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc);
        void self_destruct();
        void reload_shaders();
        void load_scene(const std::string& filename);

    public:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage storage;
        Swapchain swapchain;
        Scene scene;
        UI ui;
        std::vector<Synchronization> syncs;
        std::vector<DeviceTimer> timers;
        Pipeline lighting_pipeline;
        DescriptorSetHandler lighting_dsh;

        void draw_frame(GameState& gs);
        vk::Extent2D recreate_swapchain();

    private:
        float total_time = 0.0f;

        void create_lighting_pipeline();
        void create_lighting_descriptor_sets();
        void record_graphics_command_buffer(uint32_t image_idx, GameState& gs);
        void submit(uint32_t image_idx, GameState& gs);
    };
} // namespace ve
