#pragma once

#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "Window.hpp"
#include "vk/LogicalDevice.hpp"
#include "vk/PhysicalDevice.hpp"

namespace ve
{
    class VulkanMainContext
    {
    public:
        explicit VulkanMainContext() : instance({}), physical_device(instance, surface), logical_device(physical_device, queues_family_indices, queues)
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Created VulkanMainContext\n");
        }

        VulkanMainContext(const uint32_t width, const uint32_t height) : window(std::make_optional<Window>(width, height)), instance(window->get_required_extensions()), surface(window->create_surface(instance.get())), physical_device(instance, surface), logical_device(physical_device, queues_family_indices, queues)
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Created VulkanMainContext\n");
        }

        void self_destruct()
        {
            if (surface.has_value()) instance.get().destroySurfaceKHR(surface.value());
            logical_device.self_destruct();
            instance.self_destruct();
            if (window.has_value()) window->self_destruct();
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Destroyed VulkanMainContext\n");
        }

        std::vector<vk::SurfaceFormatKHR> get_surface_formats() const
        {
            if (!surface.has_value()) VE_THROW("Surface not initialized!\n");
            return physical_device.get().getSurfaceFormatsKHR(surface.value());
        }

        std::vector<vk::PresentModeKHR> get_surface_present_modes() const
        {
            if (!surface.has_value()) VE_THROW("Surface not initialized!\n");
            return physical_device.get().getSurfacePresentModesKHR(surface.value());
        }

        vk::SurfaceCapabilitiesKHR get_surface_capabilities() const
        {
            if (!surface.has_value()) VE_THROW("Surface not initialized!\n");
            return physical_device.get().getSurfaceCapabilitiesKHR(surface.value());
        }

        const vk::Queue& get_graphics_queue() const
        {
            return queues.at(QueueIndex::Graphics);
        }

        const vk::Queue& get_transfer_queue() const
        {
            return queues.at(QueueIndex::Transfer);
        }

        const vk::Queue& get_compute_queue() const
        {
            return queues.at(QueueIndex::Compute);
        }

        const vk::Queue& get_present_queue() const
        {
            return queues.at(QueueIndex::Present);
        }

    private:
        std::unordered_map<QueueIndex, vk::Queue> queues;

    public:
        QueueFamilyIndices queues_family_indices;
        std::optional<Window> window;
        Instance instance;
        std::optional<vk::SurfaceKHR> surface;
        PhysicalDevice physical_device;
        LogicalDevice logical_device;
    };
}// namespace ve