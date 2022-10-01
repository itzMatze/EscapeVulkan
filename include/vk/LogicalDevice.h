#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.h"
#include "vk/PhysicalDevice.h"

namespace ve
{
    class LogicalDevice
    {
    public:
        LogicalDevice(const PhysicalDevice& physical_device)
        {
            VE_LOG_CONSOLE("Creating logical device");
            QueueFamilyIndices indices = physical_device.find_queue_families();

            vk::DeviceQueueCreateInfo qci{};
            qci.sType = vk::StructureType::eDeviceQueueCreateInfo;
            qci.queueFamilyIndex = indices.graphics;
            qci.queueCount = 1;
            float queue_prio = 1.0f;
            qci.pQueuePriorities = &queue_prio;

            vk::PhysicalDeviceFeatures device_features{};
            vk::DeviceCreateInfo dci{};
            dci.sType = vk::StructureType::eDeviceCreateInfo;
            dci.pQueueCreateInfos = &qci;
            dci.queueCreateInfoCount = 1;
            
            VE_CHECK(physical_device.get().createDevice(&dci, nullptr, &device), "Failed to create logical device!");
            queues.push_back(device.getQueue(indices.graphics, 0));
        }

        ~LogicalDevice()
        {
            device.destroy();
        }

        vk::Queue get_graphics_queue()
        {
            return queues[0];
        }

    private:
        std::vector<vk::Queue> queues;
        vk::Device device;
    };
}// namespace ve
