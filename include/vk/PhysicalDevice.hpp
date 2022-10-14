#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>

#include "vk/ExtensionsHandler.hpp"
#include "vk/Instance.hpp"

namespace ve
{
    struct QueueFamilyIndices {
        QueueFamilyIndices() : graphics(-1), compute(-1), transfer(-1), present(-1)
        {}
        int32_t graphics;
        int32_t compute;
        int32_t transfer;
        int32_t present;
    };

    class PhysicalDevice
    {
    public:
        PhysicalDevice(const Instance& instance, const std::optional<vk::SurfaceKHR>& surface);
        vk::PhysicalDevice get() const;
        QueueFamilyIndices get_queue_families() const;
        const std::vector<const char*>& get_extensions() const;
        const std::vector<const char*>& get_missing_extensions();

    private:
        vk::PhysicalDevice physical_device;
        QueueFamilyIndices queue_family_indices;
        ExtensionsHandler extensions_handler;

        void find_queue_families(const std::optional<vk::SurfaceKHR>& surface);
        bool is_device_suitable(uint32_t idx, const vk::PhysicalDevice p_device, const std::optional<vk::SurfaceKHR>& surface);
        bool is_swapchain_supported(const vk::PhysicalDevice p_device, const vk::SurfaceKHR& surface) const;
        int32_t get_queue_score(vk::QueueFamilyProperties queue_family, vk::QueueFlagBits target) const;
    };
}// namespace ve
