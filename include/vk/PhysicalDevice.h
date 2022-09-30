#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.h"

namespace ve
{
    class PhysicalDevice
    {
    public:
        PhysicalDevice(const Instance& instance)
        {
            std::vector<vk::PhysicalDevice> physical_devices = instance.get_physical_devices();
            std::unordered_set<uint32_t> suitable_p_devices;
            VE_LOG_CONSOLE_START("Found physical devices: \n");
            for (uint32_t i = 0; i < physical_devices.size(); ++i)
            {
                vk::PhysicalDeviceProperties pdp;
                physical_devices[i].getProperties(&pdp);
                vk::PhysicalDeviceFeatures pdf;
                physical_devices[i].getFeatures(&pdf);
                VE_CONSOLE_ADD("    " << i << " " << pdp.deviceName << " ");
                if (is_device_suitable(pdp, pdf))
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
            }
            else if (suitable_p_devices.size() == 1)
            {
                VE_CONSOLE_END("Only one suitable GPU. Using that one.\n");
            }
            else
            {
                VE_CONSOLE_END("");
                VE_THROW("No suitable GPUs found!");
            }
        }

    private:
        bool is_device_suitable(vk::PhysicalDeviceProperties pdp, vk::PhysicalDeviceFeatures pdf)
        {
            return pdp.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
        }
        vk::PhysicalDevice physical_device;
    };
}// namespace ve
