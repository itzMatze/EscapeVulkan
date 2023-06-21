#pragma once

#include <optional>

#include "vk/common.hpp"
#include "Window.hpp"
#include "vk/LogicalDevice.hpp"
#include "vk/PhysicalDevice.hpp"
#include "vk_mem_alloc.h"

namespace ve
{
    class VulkanMainContext
    {
    public:
        explicit VulkanMainContext();
        VulkanMainContext(const uint32_t width, const uint32_t height);
        void self_destruct();
        std::vector<vk::SurfaceFormatKHR> get_surface_formats() const;
        std::vector<vk::PresentModeKHR> get_surface_present_modes() const;
        vk::SurfaceCapabilitiesKHR get_surface_capabilities() const;
        const vk::Queue& get_graphics_queue() const;
        const vk::Queue& get_transfer_queue() const;
        const vk::Queue& get_compute_queue() const;
        const vk::Queue& get_present_queue() const;

    private:
        std::unordered_map<QueueIndex, vk::Queue> queues;

        void create_vma_allocator();
        void setup_debug_messenger();

    public:
        std::optional<Window> window;
        Instance instance;
        vk::DebugUtilsMessengerEXT debug_messenger;
        std::optional<vk::SurfaceKHR> surface;
        PhysicalDevice physical_device;
        QueueFamilyIndices queue_family_indices;
        LogicalDevice logical_device;
        VmaAllocator va;
    };
} // namespace ve
