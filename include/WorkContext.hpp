#pragma once

#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "UI.hpp"
#include "vk/common.hpp"
#include "vk/Buffer.hpp"
#include "vk/Scene.hpp"
#include "vk/TunnelObjects.hpp"
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
        TunnelObjects tunnel;
        std::vector<Synchronization> syncs;
        std::vector<DeviceTimer> timers;

        void draw_frame(DrawInfo& di);
        vk::Extent2D recreate_swapchain();

    private:
        float total_time = 0.0f;

        void record_graphics_command_buffer(uint32_t image_idx, DrawInfo& di);
        void submit(uint32_t image_idx, DrawInfo& di);
    };
} // namespace ve
