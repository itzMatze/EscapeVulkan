#pragma once

#include "vk/common.hpp"
#include "vk/Image.hpp"
#include "vk/RenderPass.hpp"
#include "vk/VulkanMainContext.hpp"
#include "Storage.hpp"

namespace ve
{
    class Swapchain
    {
    public:
        Swapchain(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void self_destruct(bool full);
        const vk::SwapchainKHR& get() const;
        const RenderPass& get_render_pass() const;
        const RenderPass& get_deferred_render_pass() const;
        vk::Extent2D get_extent() const;
        vk::Framebuffer get_framebuffer(uint32_t idx) const;
        vk::Framebuffer get_deferred_framebuffer() const;
        void construct();
        void save_screenshot(VulkanCommandContext& vcc, uint32_t image_idx, uint32_t current_frame);

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        vk::Extent2D extent;
        vk::SurfaceFormatKHR surface_format;
        vk::Format depth_format;
        vk::SwapchainKHR swapchain;
        RenderPass render_pass;
        uint32_t depth_buffer;
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> image_views;
        std::vector<vk::Framebuffer> framebuffers;

        RenderPass deferred_render_pass;
        uint32_t deferred_depth_buffer;
        std::vector<uint32_t> deferred_images;
        vk::Framebuffer deferred_framebuffer;

        vk::SwapchainKHR create_swapchain();
        void create_framebuffers();
        vk::PresentModeKHR choose_present_mode();
        vk::Extent2D choose_extent();
        vk::SurfaceFormatKHR choose_surface_format();
        vk::Format choose_depth_format();
    };
} // namespace ve
