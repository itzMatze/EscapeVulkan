#pragma once

#include <optional>

#include "vk/common.hpp"
#include "vk/ExtensionsHandler.hpp"
#include "vk/Instance.hpp"

namespace ve
{
    struct QueueFamilyIndices {
        QueueFamilyIndices() : graphics(0), compute(0), transfer(0), present(0)
        {}
        QueueFamilyIndices(uint32_t value) : graphics(value), compute(value), transfer(value), present(value)
        {}
        uint32_t graphics;
        uint32_t compute;
        uint32_t transfer;
        uint32_t present;
    };

    class PhysicalDevice
    {
    public:
        PhysicalDevice(const Instance& instance, const std::optional<vk::SurfaceKHR>& surface);
        vk::PhysicalDevice get() const;
        QueueFamilyIndices get_queue_families(const std::optional<vk::SurfaceKHR>& surface) const;
        const std::vector<const char*>& get_extensions() const;
        const std::vector<const char*>& get_missing_extensions();

    private:
        vk::PhysicalDevice physical_device;
        ExtensionsHandler extensions_handler;

        bool is_device_suitable(uint32_t idx, const vk::PhysicalDevice p_device, const std::optional<vk::SurfaceKHR>& surface);
        bool is_swapchain_supported(const vk::PhysicalDevice p_device, const vk::SurfaceKHR& surface) const;
        int32_t get_queue_score(vk::QueueFamilyProperties queue_family, vk::QueueFlagBits target) const;
    };
} // namespace ve
