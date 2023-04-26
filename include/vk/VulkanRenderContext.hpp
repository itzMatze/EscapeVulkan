#pragma once

#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Camera.hpp"
#include "UI.hpp"
#include "vk/common.hpp"
#include "vk/Buffer.hpp"
#include "vk/DescriptorSetHandler.hpp"
#include "vk/Scene.hpp"
#include "vk/Swapchain.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanStorageContext.hpp"

namespace ve
{
    class VulkanRenderContext
    {
    public:
        VulkanRenderContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc, VulkanStorageContext& vsc);
        void self_destruct();
        void load_scene(const std::string& filename);

    private:
        enum class SyncNames
        {
            SImageAvailable,
            SRenderFinished,
            FRenderFinished
        };

    public:
        const uint32_t frames_in_flight = 2;
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        VulkanStorageContext& vsc;
        std::unordered_map<SyncNames, std::vector<uint32_t>> sync_indices;
        Swapchain swapchain;
        Scene scene;
        UI ui;

        void draw_frame(DrawInfo& di);
        vk::Extent2D recreate_swapchain();

    private:
        float total_time = 0.0f;

        void record_graphics_command_buffer(uint32_t image_idx, DrawInfo& di);
        void submit_graphics(uint32_t image_idx, DrawInfo& di);
    };
} // namespace ve
