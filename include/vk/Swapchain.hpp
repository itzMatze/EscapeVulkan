#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"
#include "vk/PhysicalDevice.hpp"
#include "vk/RenderPass.hpp"

namespace ve
{
    class Swapchain
    {
    public:
        Swapchain(const PhysicalDevice& physical_device, const vk::Device logical_device, const vk::SurfaceKHR surface, SDL_Window* window) : device(logical_device)
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "swapchain +\n");
            std::vector<vk::SurfaceFormatKHR> formats = physical_device.get_surface_formats(surface);
            vk::SurfaceFormatKHR surface_format = choose_surface_format(formats);
            format = surface_format.format;
            std::vector<vk::PresentModeKHR> present_modes = physical_device.get_surface_present_modes(surface);
            vk::SurfaceCapabilitiesKHR capabilities = physical_device.get().getSurfaceCapabilitiesKHR(surface);
            extent = choose_extent(capabilities, window);
            uint32_t image_count = capabilities.maxImageCount > 0 ? std::min(capabilities.minImageCount + 1, capabilities.maxImageCount) : capabilities.minImageCount + 1;
            vk::SwapchainCreateInfoKHR sci{};
            sci.sType = vk::StructureType::eSwapchainCreateInfoKHR;
            sci.surface = surface;
            sci.minImageCount = image_count;
            sci.imageFormat = surface_format.format;
            sci.imageColorSpace = surface_format.colorSpace;
            sci.imageExtent = extent;
            sci.imageArrayLayers = 1;
            sci.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
            sci.preTransform = capabilities.currentTransform;
            sci.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
            sci.presentMode = choose_present_mode(present_modes);
            sci.clipped = VK_TRUE;
            sci.oldSwapchain = VK_NULL_HANDLE;
            QueueFamilyIndices indices = physical_device.get_queue_families();
            uint32_t queue_family_indices[] = {static_cast<uint32_t>(indices.graphics), static_cast<uint32_t>(indices.present)};
            if (indices.graphics != indices.present)
            {
                VE_LOG_CONSOLE(VE_DEBUG, "Graphics and Presentation queue are two distinct queues. Using Concurrent sharing mode on swapchain.\n");
                sci.imageSharingMode = vk::SharingMode::eConcurrent;
                sci.queueFamilyIndexCount = 2;
                sci.pQueueFamilyIndices = queue_family_indices;
            }
            else
            {
                VE_LOG_CONSOLE(VE_DEBUG, "Graphics and Presentation queue are the same queue. Using Exclusive sharing mode on swapchain.\n");
                sci.imageSharingMode = vk::SharingMode::eExclusive;
            }
            VE_LOG_CONSOLE(VE_INFO, "Creating swapchain\n");
            swapchain = device.createSwapchainKHR(sci);
            images = device.getSwapchainImagesKHR(swapchain);

            VE_LOG_CONSOLE(VE_INFO, "Creating image views\n");
            for (const auto& image: images)
            {
                vk::ImageViewCreateInfo ivci{};
                ivci.sType = vk::StructureType::eImageViewCreateInfo;
                ivci.image = image;
                ivci.viewType = vk::ImageViewType::e2D;
                ivci.format = format;
                ivci.components.r = vk::ComponentSwizzle::eIdentity;
                ivci.components.g = vk::ComponentSwizzle::eIdentity;
                ivci.components.b = vk::ComponentSwizzle::eIdentity;
                ivci.components.a = vk::ComponentSwizzle::eIdentity;
                ivci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                ivci.subresourceRange.baseMipLevel = 0;
                ivci.subresourceRange.levelCount = 1;
                ivci.subresourceRange.baseArrayLayer = 0;
                ivci.subresourceRange.layerCount = 1;
                image_views.push_back(device.createImageView(ivci));
            }

            render_pass = RenderPass(device, format);

            VE_LOG_CONSOLE(VE_INFO, "Creating framebuffers\n");
            for (const auto& image_view: image_views)
            {
                vk::FramebufferCreateInfo fbci{};
                fbci.sType = vk::StructureType::eFramebufferCreateInfo;
                fbci.renderPass = render_pass.get();
                fbci.attachmentCount = 1;
                fbci.pAttachments = &image_view;
                fbci.width = extent.width;
                fbci.height = extent.height;
                fbci.layers = 1;

                framebuffers.push_back(device.createFramebuffer(fbci));
            }

            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "swapchain +++\n");
        }

        ~Swapchain()
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Swapchain -\n");
            VE_LOG_CONSOLE(VE_INFO, "Destroying framebuffers\n");
            for (auto& framebuffer: framebuffers)
            {
                device.destroyFramebuffer(framebuffer);
            }
            render_pass.self_destruct(device);
            VE_LOG_CONSOLE(VE_INFO, "Destroying image views\n");
            for (auto& image_view: image_views)
            {
                device.destroyImageView(image_view);
            }
            VE_LOG_CONSOLE(VE_INFO, "Destroying swapchain\n");
            device.destroySwapchainKHR(swapchain);
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Swapchain ---\n");
        }

        vk::SwapchainKHR get() const
        {
            return swapchain;
        }

        vk::Extent2D get_extent() const
        {
            return extent;
        }

        vk::Format get_format() const
        {
            return format;
        }

        vk::RenderPass get_render_pass()
        {
            return render_pass.get();
        }

    private:
        vk::SurfaceFormatKHR choose_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats)
        {
            for (const auto& format: available_formats)
            {
                if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) return format;
            }
            VE_LOG_CONSOLE(VE_WARN, VE_C_YELLOW << "Desired format not found. Using first available.");
            return available_formats[0];
        }

        vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes)
        {
            for (const auto& pm: available_present_modes)
            {
                if (pm == vk::PresentModeKHR::eImmediate) return pm;
            }
            VE_LOG_CONSOLE(VE_WARN, VE_C_YELLOW << "Desired present mode not found. Using FIFO.");
            return vk::PresentModeKHR::eFifo;
        }

        vk::Extent2D choose_extent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window)
        {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            {
                return capabilities.currentExtent;
            }
            else
            {
                int32_t width, height;
                SDL_Vulkan_GetDrawableSize(window, &width, &height);
                vk::Extent2D extent(width, height);
                extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
                return extent;
            }
        }
        const vk::Device device;
        vk::SwapchainKHR swapchain;
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> image_views;
        std::vector<vk::Framebuffer> framebuffers;
        RenderPass render_pass;
        vk::Format format;
        vk::Extent2D extent;
    };
}// namespace ve