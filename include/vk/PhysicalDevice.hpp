#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>

#include "ve_log.hpp"
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
        PhysicalDevice(const Instance& instance, const std::optional<vk::SurfaceKHR>& surface)
        {
            const std::vector<const char*> required_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
            const std::vector<const char*> optional_extensions{VK_KHR_RAY_QUERY_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};
            extensions_handler.add_extensions(required_extensions, true);
            extensions_handler.add_extensions(optional_extensions, false);

            std::vector<vk::PhysicalDevice> physical_devices = instance.get_physical_devices();
            std::unordered_set<uint32_t> suitable_p_devices;
            for (uint32_t i = 0; i < physical_devices.size(); ++i)
            {
                if (is_device_suitable(i, physical_devices[i], surface))
                {
                    suitable_p_devices.insert(i);
                }
            }
            if (suitable_p_devices.size() > 1)
            {
                uint32_t pd_idx = 0;
                do
                {
                    VE_TO_USER("Select one of the suitable GPUs by typing the number\n");
                    std::cin >> pd_idx;
                } while (!suitable_p_devices.contains(pd_idx));
                physical_device = physical_devices[pd_idx];
            }
            else if (suitable_p_devices.size() == 1)
            {
                VE_CONSOLE_ADD(VE_DEBUG, "Only one suitable GPU. Using that one.\n");
                physical_device = physical_devices[*(suitable_p_devices.begin())];
            }
            else
            {
                VE_THROW("No suitable GPUs found!");
            }
            extensions_handler.remove_missing_extensions();

            find_queue_families(surface);
            VE_LOG_CONSOLE(VE_DEBUG, "Queue family indices: \n    Graphics: " << queue_family_indices.graphics << "\n    Compute:  " << queue_family_indices.compute << "\n    Transfer: " << queue_family_indices.transfer << "\n    Present:  " << queue_family_indices.present << "\n");
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
        void find_queue_families(const std::optional<vk::SurfaceKHR>& surface)
        {
            std::vector<vk::QueueFamilyProperties> queue_families = physical_device.getQueueFamilyProperties();
            QueueFamilyIndices scores;
            for (uint32_t i = 0; i < queue_families.size(); ++i)
            {
                if (surface.has_value())
                {
                    vk::Bool32 present_support = false;
                    present_support = physical_device.getSurfaceSupportKHR(i, surface.value());
                    // take what we get for present queue, but ideally present and graphics queue are the same
                    if (present_support && scores.present < 0)
                    {
                        scores.present = 0;
                        queue_family_indices.present = i;
                    }
                    if (scores.graphics < get_queue_score(queue_families[i], vk::QueueFlagBits::eGraphics))
                    {
                        if (present_support && scores.present < 1)
                        {
                            scores.present = 1;
                            queue_family_indices.present = i;
                        }
                        scores.graphics = get_queue_score(queue_families[i], vk::QueueFlagBits::eGraphics);
                        queue_family_indices.graphics = i;
                    }
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

        bool is_device_suitable(uint32_t idx, const vk::PhysicalDevice p_device, const std::optional<vk::SurfaceKHR>& surface)
        {
            vk::PhysicalDeviceProperties pdp = p_device.getProperties();
            vk::PhysicalDeviceFeatures p_device_features = p_device.getFeatures();
            std::vector<vk::ExtensionProperties> available_extensions = p_device.enumerateDeviceExtensionProperties();
            std::vector<const char*> avail_ext_names;
            for (const auto& ext: available_extensions) avail_ext_names.push_back(ext.extensionName);
            VE_TO_USER("    " << idx << " " << pdp.deviceName << " ");
            int32_t missing_extensions = extensions_handler.check_extension_availability(avail_ext_names);
            if (missing_extensions == -1 || !p_device_features.samplerAnisotropy || (surface.has_value() && extensions_handler.find_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME) && !is_swapchain_supported(p_device, surface.value())))
            {
                VE_TO_USER("(not suitable)\n");
                return false;
            }
            VE_TO_USER("(suitable, " << missing_extensions << " missing optional extensions)\n");
            return true;
        }

        bool
        is_swapchain_supported(const vk::PhysicalDevice p_device, const vk::SurfaceKHR& surface) const
        {
            std::vector<vk::SurfaceFormatKHR> f = p_device.getSurfaceFormatsKHR(surface);
            std::vector<vk::PresentModeKHR> pm = p_device.getSurfacePresentModesKHR(surface);
            return !f.empty() && !pm.empty();
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

        vk::PhysicalDevice physical_device;
        QueueFamilyIndices queue_family_indices;
        ExtensionsHandler extensions_handler;
    };
}// namespace ve
