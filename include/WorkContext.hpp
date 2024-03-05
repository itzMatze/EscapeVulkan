#pragma once

#include <glm/mat4x4.hpp>
#include <vector>

#include "UI.hpp"
#include "vk/common.hpp"
#include "vk/Scene.hpp"
#include "vk/Swapchain.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk/VulkanMainContext.hpp"
#include "Storage.hpp"
#include "vk/Timer.hpp"
#include "vk/Lighting.hpp"

namespace ve
{
class WorkContext
{
public:
    WorkContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc);
    void self_destruct();
    void reload_shaders();
    void load_scene(const std::string& filename);
    void restart();

public:
    const VulkanMainContext& vmc;
    VulkanCommandContext& vcc;
    Storage storage;
    Swapchain swapchain;
    Scene scene;
    UI ui;
    Lighting lighting;
    std::vector<Synchronization> syncs;
    std::vector<DeviceTimer> timers;

    void draw_frame(GameState& gs);
    vk::Extent2D recreate_swapchain();

private:
    void record_graphics_command_buffer(uint32_t image_idx, GameState& gs);
    void submit(uint32_t image_idx, GameState& gs);
};
} // namespace ve
