#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.h"
#include "vk/ExtensionsHandler.h"

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
        PhysicalDevice(const Instance& instance, const std::vector<const char*>& required_extensions, const std::vector<const char*> optional_extensions)
        {
            VE_LOG_CONSOLE("Creating physical device");
            std::vector<vk::PhysicalDevice> physical_devices = instance.get_physical_devices();
            std::unordered_set<uint32_t> suitable_p_devices;
            VE_LOG_CONSOLE_START("Found physical devices: \n");
            for (uint32_t i = 0; i < physical_devices.size(); ++i)
            {
                if (is_device_suitable(i, physical_devices[i], required_extensions, optional_extensions))
                {
                    suitable_p_devices.insert(i);
                }
            }
            if (suitable_p_devices.size() > 1)
            {
                uint32_t pd_idx = 0;
                do
                {
                    VE_CONSOLE_END("Select one of the suitable GPUs by typing the number\n");
                    std::cin >> pd_idx;
                } while (!suitable_p_devices.contains(pd_idx));
                physical_device = physical_devices[pd_idx];
            }
            else if (suitable_p_devices.size() == 1)
            {
                VE_CONSOLE_END("Only one suitable GPU. Using that one.\n");
                physical_device = physical_devices[*(suitable_p_devices.begin())];
            }
            else
            {
                VE_CONSOLE_END("");
                VE_THROW("No suitable GPUs found!");
            }
            extensions_handler.add_extensions(get_device_extensions(physical_device), required_extensions, true);
            extensions_handler.add_extensions(get_device_extensions(physical_device), optional_extensions, false);
            find_queue_families(instance.get_surface());
        }

        vk::PhysicalDevice get() const
        {
            return physical_device;
        }

        QueueFamilyIndices get_queue_families() const
        {
            return queue_family_indices;
        }

        const std::vector<const char*>& get_extensions() const
        {
            return extensions_handler.get_extensions();
        }

        const std::vector<const char*>& get_missing_extensions()
        {
            return extensions_handler.get_missing_extensions();
        }

    private:
        void find_queue_families(const vk::SurfaceKHR surface)
        {
            uint32_t queue_family_count = 0;
            physical_device.getQueueFamilyProperties(&queue_family_count, nullptr);
            std::vector<vk::QueueFamilyProperties> queue_families(queue_family_count);
            physical_device.getQueueFamilyProperties(&queue_family_count, queue_families.data());
            QueueFamilyIndices scores;
            for (uint32_t i = 0; i < queue_families.size(); ++i)
            {
                vk::Bool32 present_support = false;
                VE_CHECK(physical_device.getSurfaceSupportKHR(i, surface, &present_support), "Failed to check surface support!");
                if (present_support)
                {
                    queue_family_indices.present = i;
                }
                if (scores.graphics < get_queue_score(queue_families[i], vk::QueueFlagBits::eGraphics))
                {
                    scores.graphics = get_queue_score(queue_families[i], vk::QueueFlagBits::eGraphics);
                    queue_family_indices.graphics = i;
                }
                if (scores.compute < get_queue_score(queue_families[i], vk::QueueFlagBits::eCompute))
                {
                    scores.compute = get_queue_score(queue_families[i], vk::QueueFlagBits::eCompute);
                    queue_family_indices.compute = i;
                }
                if (scores.transfer < get_queue_score(queue_families[i], vk::QueueFlagBits::eTransfer))
                {
                    scores.transfer = get_queue_score(queue_families[i], vk::QueueFlagBits::eTransfer);
                    queue_family_indices.transfer = i;
                }
            }
            VE_ASSERT(queue_family_indices.graphics > -1 && queue_family_indices.compute > -1 && queue_family_indices.transfer > -1, "One queue family could not be satisfied!");
        }

        bool is_device_suitable(uint32_t idx, const vk::PhysicalDevice p_device, const std::vector<const char*>& required_extensions, const std::vector<const char*>& optional_extensions) const
        {
            vk::PhysicalDeviceProperties pdp;
            p_device.getProperties(&pdp);
            vk::PhysicalDeviceFeatures p_device_features;
            p_device.getFeatures(&p_device_features);
            std::vector<vk::ExtensionProperties> available_extensions = get_device_extensions(p_device);

            VE_CONSOLE_ADD("    " << idx << " " << pdp.deviceName << " ");
            for (const auto& requested_extension: required_extensions)
            {
                if (!extensions_handler.is_extension_available(requested_extension, available_extensions))
                {
                    VE_CONSOLE_ADD("(not suitable)\n");
                    return false;
                }
            }

            uint32_t missing_optional_extensions = 0;
            for (const auto& requested_extension: optional_extensions)
            {
                if (!extensions_handler.is_extension_available(requested_extension, available_extensions)) ++missing_optional_extensions;
            }

            VE_CONSOLE_ADD("(suitable, " << missing_optional_extensions << " missing optional extensions)\n");
            return pdp.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
        }

        int32_t get_queue_score(vk::QueueFamilyProperties queue_family, vk::QueueFlagBits target) const
        {
            // required queue family not supported by this queue
            if (!(queue_family.queueFlags & target)) return -1;
            int32_t score = 0;
            // every missing queue feature increases score, as this means that the queue is more specialized
            if (!(queue_family.queueFlags & vk::QueueFlagBits::eGraphics)) ++score;
            if (!(queue_family.queueFlags & vk::QueueFlagBits::eCompute)) ++score;
            if (!(queue_family.queueFlags & vk::QueueFlagBits::eProtected)) ++score;
            if (!(queue_family.queueFlags & vk::QueueFlagBits::eTransfer)) ++score;
            if (!(queue_family.queueFlags & vk::QueueFlagBits::eSparseBinding)) ++score;
            return score;
        }

        std::vector<vk::ExtensionProperties> get_device_extensions(vk::PhysicalDevice p_device) const
        {
            uint32_t extension_count;
            VE_CHECK(p_device.enumerateDeviceExtensionProperties(nullptr, &extension_count, nullptr), "Failed to get physical device extension count!");
            std::vector<vk::ExtensionProperties> available_extensions(extension_count);
            VE_CHECK(p_device.enumerateDeviceExtensionProperties(nullptr, &extension_count, available_extensions.data()), "Failed to get physical device extensions!");
            return available_extensions;
        }

        vk::PhysicalDevice physical_device;
        QueueFamilyIndices queue_family_indices;
        ExtensionsHandler extensions_handler;
    };
}// namespace ve
