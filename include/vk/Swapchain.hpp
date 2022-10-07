#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"
#include "vk/PhysicalDevice.hpp"
#include "vk/RenderPass.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class Swapchain
    {
    public:
        Swapchain(const VulkanMainContext& vmc, const vk::SurfaceFormatKHR& surface_format, const vk::RenderPass& render_pass) : vmc(vmc)
        {
            create_swapchain(surface_format, render_pass);
        }

        const vk::SwapchainKHR& get() const
        {
            return swapchain;
        }

        vk::Extent2D get_extent() const
        {
            return extent;
        }

        vk::Framebuffer get_framebuffer(uint32_t idx) const
        {
            return framebuffers[idx];
        }

        void create_swapchain(const vk::SurfaceFormatKHR& surface_format, const vk::RenderPass& render_pass)
        {
            std::vector<vk::PresentModeKHR> present_modes = vmc.get_surface_present_modes();
            vk::SurfaceCapabilitiesKHR capabilities = vmc.get_surface_capabilities();
            choose_extent(capabilities);
            uint32_t image_count = capabilities.maxImageCount > 0 ? std::min(capabilities.minImageCount + 1, capabilities.maxImageCount) : capabilities.minImageCount + 1;
            vk::SwapchainCreateInfoKHR sci{};
            sci.sType = vk::StructureType::eSwapchainCreateInfoKHR;
            sci.surface = vmc.surface;
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
            QueueFamilyIndices indices = vmc.physical_device.get_queue_families();
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
            swapchain = vmc.logical_device.get().createSwapchainKHR(sci);
            images = vmc.logical_device.get().getSwapchainImagesKHR(swapchain);

            for (const auto& image: images)
            {
                vk::ImageViewCreateInfo ivci{};
                ivci.sType = vk::StructureType::eImageViewCreateInfo;
                ivci.image = image;
                ivci.viewType = vk::ImageViewType::e2D;
                ivci.format = surface_format.format;
                ivci.components.r = vk::ComponentSwizzle::eIdentity;
                ivci.components.g = vk::ComponentSwizzle::eIdentity;
                ivci.components.b = vk::ComponentSwizzle::eIdentity;
                ivci.components.a = vk::ComponentSwizzle::eIdentity;
                ivci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                ivci.subresourceRange.baseMipLevel = 0;
                ivci.subresourceRange.levelCount = 1;
                ivci.subresourceRange.baseArrayLayer = 0;
                ivci.subresourceRange.layerCount = 1;
                image_views.push_back(vmc.logical_device.get().createImageView(ivci));
            }

            for (const auto& image_view: image_views)
            {
                vk::FramebufferCreateInfo fbci{};
                fbci.sType = vk::StructureType::eFramebufferCreateInfo;
                fbci.renderPass = render_pass;
                fbci.attachmentCount = 1;
                fbci.pAttachments = &image_view;
                fbci.width = extent.width;
                fbci.height = extent.height;
                fbci.layers = 1;

                framebuffers.push_back(vmc.logical_device.get().createFramebuffer(fbci));
            }
        }

        void self_destruct()
        {
            for (auto& framebuffer: framebuffers)
            {
                vmc.logical_device.get().destroyFramebuffer(framebuffer);
            }
            framebuffers.clear();
            for (auto& image_view: image_views)
            {
                vmc.logical_device.get().destroyImageView(image_view);
            }
            image_views.clear();
            vmc.logical_device.get().destroySwapchainKHR(swapchain);
        }

    private:
        vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes)
        {
            for (const auto& pm: available_present_modes)
            {
                if (pm == vk::PresentModeKHR::eImmediate) return pm;
            }
            VE_LOG_CONSOLE(VE_WARN, VE_C_YELLOW << "Desired present mode not found. Using FIFO.");
            return vk::PresentModeKHR::eFifo;
        }

        void choose_extent(const vk::SurfaceCapabilitiesKHR& capabilities)
        {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            {
                extent = capabilities.currentExtent;
            }
            else
            {
                int32_t width, height;
                SDL_Event e;
                do
                {
                    SDL_Vulkan_GetDrawableSize(vmc.window.get(), &width, &height);
                    SDL_WaitEvent(&e);
                } while (width == 0 || height == 0);
                vk::Extent2D extent(width, height);
                extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            }
        }
        const VulkanMainContext& vmc;
        vk::Extent2D extent;
        vk::SwapchainKHR swapchain;
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> image_views;
        std::vector<vk::Framebuffer> framebuffers;
    };
}// namespace ve