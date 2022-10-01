#pragma once

#include <set>
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
            QueueFamilyIndices indices = physical_device.get_queue_families();

            std::vector<vk::DeviceQueueCreateInfo> qci_s;
            std::set<int32_t> unique_queue_families = {indices.graphics, indices.compute, indices.transfer, indices.present};
            float queue_prio = 1.0f;
            for (uint32_t queue_family: unique_queue_families)
            {
                vk::DeviceQueueCreateInfo qci{};
                qci.sType = vk::StructureType::eDeviceQueueCreateInfo;
                qci.queueFamilyIndex = queue_family;
                qci.queueCount = 1;
                qci.pQueuePriorities = &queue_prio;
                qci_s.push_back(qci);
            }

            vk::PhysicalDeviceFeatures device_features{};
            vk::DeviceCreateInfo dci{};
            dci.sType = vk::StructureType::eDeviceCreateInfo;
            dci.queueCreateInfoCount = qci_s.size();
            dci.pQueueCreateInfos = qci_s.data();
            dci.enabledExtensionCount = physical_device.get_extensions().size();
            dci.ppEnabledExtensionNames = physical_device.get_extensions().data();

            device = physical_device.get().createDevice(dci);
            queues.resize(4);
            queues_indices.graphics = 0;
            queues[queues_indices.graphics] = device.getQueue(indices.graphics, 0);
            queues_indices.compute = 1;
            queues[queues_indices.compute] = device.getQueue(indices.compute, 0);
            queues_indices.transfer = 2;
            queues[queues_indices.transfer] = device.getQueue(indices.transfer, 0);
            queues_indices.present = 3;
            queues[queues_indices.present] = device.getQueue(indices.present, 0);
        }

        ~LogicalDevice()
        {
            device.destroy();
        }

        vk::Queue get_graphics_queue() const
        {
            return queues[queues_indices.graphics];
        }

        vk::Queue get_compute_queue() const
        {
            return queues[queues_indices.compute];
        }

        vk::Queue get_transfer_queue() const
        {
            return queues[queues_indices.transfer];
        }

        vk::Queue get_present_queue() const
        {
            return queues[queues_indices.present];
        }

    private:
        QueueFamilyIndices queues_indices;
        std::vector<vk::Queue> queues;
        vk::Device device;
    };
}// namespace ve
