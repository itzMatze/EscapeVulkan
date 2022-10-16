#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/RenderPass.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/Image.hpp"

namespace ve
{
    class Swapchain
    {
    public:
        Swapchain(const VulkanMainContext& vmc, const vk::SurfaceFormatKHR& surface_format, const vk::Format& depth_format, const vk::RenderPass& render_pass);
        const vk::SwapchainKHR& get() const;
        vk::Extent2D get_extent() const;
        vk::Framebuffer get_framebuffer(uint32_t idx) const;
        void create_swapchain(const vk::SurfaceFormatKHR& surface_format, const vk::Format& depth_format, const vk::RenderPass& render_pass);
        void self_destruct();

    private:
        const VulkanMainContext& vmc;
        vk::Extent2D extent;
        vk::SwapchainKHR swapchain;
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> image_views;
        Image depth_buffer;
        std::vector<vk::Framebuffer> framebuffers;

        vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes);
        void choose_extent(const vk::SurfaceCapabilitiesKHR& capabilities);
    };
}// namespace ve
