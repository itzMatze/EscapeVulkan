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
        void restart(GameState& gs);

    public:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage storage;
        Swapchain swapchain;
        Scene scene;
        UI ui;
        std::vector<Synchronization> syncs;
        std::vector<DeviceTimer> timers;
        std::vector<uint32_t> restir_reservoir_buffers;
        Pipeline lighting_pipeline_0;
        Pipeline lighting_pipeline_1;
        DescriptorSetHandler lighting_dsh;

        void draw_frame(GameState& gs);
        vk::Extent2D recreate_swapchain();

    private:
        void create_lighting_pipeline();
        void create_lighting_descriptor_sets();
        void display_dead_screen(uint32_t image_idx, GameState& gs);
        void record_graphics_command_buffer(uint32_t image_idx, GameState& gs);
        void submit(uint32_t image_idx, GameState& gs);
    };
} // namespace ve
