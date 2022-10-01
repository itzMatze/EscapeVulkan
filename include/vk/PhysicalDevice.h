#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.h"

namespace ve
{
    struct QueueFamilyIndices {
        QueueFamilyIndices() : graphics(-1), compute(-1), transfer(-1)
        {}
        int32_t graphics;
        int32_t compute;
        int32_t transfer;
    };

    class PhysicalDevice
    {
    public:
        PhysicalDevice(const Instance& instance)
        {
            VE_LOG_CONSOLE("Creating physical device");
            std::vector<vk::PhysicalDevice> physical_devices = instance.get_physical_devices();
            std::unordered_set<uint32_t> suitable_p_devices;
            VE_LOG_CONSOLE_START("Found physical devices: \n");
            for (uint32_t i = 0; i < physical_devices.size(); ++i)
            {
                vk::PhysicalDeviceProperties pdp;
                physical_devices[i].getProperties(&pdp);
                physical_devices[i].getFeatures(&physical_device_features);
                VE_CONSOLE_ADD("    " << i << " " << pdp.deviceName << " ");
                if (is_device_suitable(pdp))
                {
                    VE_CONSOLE_ADD("(suitable)\n");
                    suitable_p_devices.insert(i);
                }
                else
                {
                    VE_CONSOLE_ADD("(not suitable)\n");
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
        }

        vk::PhysicalDevice get() const
        {
            return physical_device;
        }

        QueueFamilyIndices find_queue_families() const
        {
            uint32_t queue_family_count = 0;
            physical_device.getQueueFamilyProperties(&queue_family_count, nullptr);
            std::vector<vk::QueueFamilyProperties> queue_families(queue_family_count);
            physical_device.getQueueFamilyProperties(&queue_family_count, queue_families.data());
            QueueFamilyIndices indices;
            QueueFamilyIndices scores;
            for (uint32_t i = 0; i < queue_families.size(); ++i)
            {
                if (scores.graphics < get_queue_score(queue_families[i], vk::QueueFlagBits::eGraphics))
                {
                    scores.graphics = get_queue_score(queue_families[i], vk::QueueFlagBits::eGraphics);
                    indices.graphics = i;
                }
                if (scores.compute < get_queue_score(queue_families[i], vk::QueueFlagBits::eCompute))
                {
                    scores.compute = get_queue_score(queue_families[i], vk::QueueFlagBits::eCompute);
                    indices.compute = i;
                }
                if (scores.transfer < get_queue_score(queue_families[i], vk::QueueFlagBits::eTransfer))
                {
                    scores.transfer = get_queue_score(queue_families[i], vk::QueueFlagBits::eTransfer);
                    indices.transfer = i;
                }
            }
            VE_ASSERT(indices.graphics > -1 && indices.compute > -1 && indices.transfer > -1, "One queue family could not be satisfied!");
            return indices;
        }

    private:
        bool is_device_suitable(vk::PhysicalDeviceProperties pdp)
        {
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

        vk::PhysicalDevice physical_device;
        vk::PhysicalDeviceFeatures physical_device_features;
    };
}// namespace ve
