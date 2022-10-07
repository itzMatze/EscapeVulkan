#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/Pipeline.hpp"
#include "vk/RenderPass.hpp"
#include "vk/Swapchain.hpp"

namespace ve
{
    struct VulkanRenderContext {
        VulkanRenderContext(const VulkanMainContext& vmc) : surface_format(choose_surface_format(vmc)), render_pass(vmc, surface_format.format), swapchain(vmc, surface_format, render_pass.get()), pipeline(vmc, render_pass.get())
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Created VulkanRenderContext\n");
        }

        void self_destruct()
        {
            pipeline.self_destruct();
            swapchain.self_destruct();
            render_pass.self_destruct();
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Destroyed VulkanRenderContext\n");
        }

        vk::SurfaceFormatKHR surface_format;
        RenderPass render_pass;
        Swapchain swapchain;
        Pipeline pipeline;

        void recreate_swapchain()
        {
            swapchain.self_destruct();
            swapchain.create_swapchain(surface_format, render_pass.get());
        }

    private:
        vk::SurfaceFormatKHR choose_surface_format(const VulkanMainContext& vmc)
        {
            std::vector<vk::SurfaceFormatKHR> formats = vmc.get_surface_formats();
            for (const auto& format: formats)
            {
                if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) return format;
            }
            VE_LOG_CONSOLE(VE_WARN, VE_C_YELLOW << "Desired format not found. Using first available.");
            return formats[0];
        }
    };
}// namespace ve