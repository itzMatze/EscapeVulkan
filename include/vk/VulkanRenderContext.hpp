#pragma once

#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Camera.hpp"
#include "UI.hpp"
#include "common.hpp"
#include "vk/Buffer.hpp"
#include "vk/DescriptorSetHandler.hpp"
#include "vk/Scene.hpp"
#include "vk/Swapchain.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    struct UniformBufferObject {
        glm::mat4 M;
    };

    class VulkanRenderContext
    {
    public:
        VulkanRenderContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc);
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
        UniformBufferObject ubo{
                glm::mat4(1.0f)};

        PushConstants pc{
                glm::mat4(1.0f)};

        const uint32_t frames_in_flight = 2;
        uint32_t current_frame = 0;
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        std::vector<ve::Buffer> uniform_buffers;
        std::unordered_map<SyncNames, std::vector<uint32_t>> sync_indices;
        Swapchain swapchain;
        Scene scene;
        UI ui;

        void draw_frame(const Camera& camera, DrawInfo& di);
        vk::Extent2D recreate_swapchain();

    private:
        float total_time = 0.0f;

        void record_graphics_command_buffer(uint32_t image_idx, const glm::mat4& vp, DrawInfo& di);
        void submit_graphics(uint32_t image_idx);
        vk::SampleCountFlagBits choose_sample_count();
    };
}// namespace ve
