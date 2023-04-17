#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/Image.hpp"
#include "vk/RenderPass.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class Swapchain
    {
    public:
        Swapchain(const VulkanMainContext& vmc);
        void self_destruct(bool full);
        const vk::SwapchainKHR& get() const;
        const RenderPass& get_render_pass() const;
        vk::Extent2D get_extent() const;
        vk::Framebuffer get_framebuffer(uint32_t idx) const;
        void create_swapchain();

    private:
        const VulkanMainContext& vmc;
        vk::Extent2D extent;
        vk::SurfaceFormatKHR surface_format;
        vk::Format depth_format;
        vk::SwapchainKHR swapchain;
        RenderPass render_pass;
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> image_views;
        Image depth_buffer;
        Image color_image;
        std::vector<vk::Framebuffer> framebuffers;

        vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes);
        void choose_extent(const vk::SurfaceCapabilitiesKHR& capabilities);
        vk::SurfaceFormatKHR choose_surface_format();
        vk::Format choose_depth_format();
    };
} // namespace ve
