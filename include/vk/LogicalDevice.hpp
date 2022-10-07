#pragma once

#include <optional>
#include <set>
#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"
#include "vk/PhysicalDevice.hpp"
#include "vk/Synchronization.hpp"

namespace ve
{
    enum QueueIndex
    {
        GRAPHICS_IDX = 0,
        COMPUTE_IDX = 1,
        TRANSFER_IDX = 2,
        PRESENT_IDX = 3
    };

    class LogicalDevice
    {
    public:
        LogicalDevice(const PhysicalDevice& p_device, QueueFamilyIndices& indices, std::vector<vk::Queue>& queues)
        {
            indices = p_device.get_queue_families();

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
            dci.enabledExtensionCount = p_device.get_extensions().size();
            dci.ppEnabledExtensionNames = p_device.get_extensions().data();

            device = p_device.get().createDevice(dci);
            queues.resize(4);
            queues[GRAPHICS_IDX] = device.getQueue(indices.graphics, 0);
            queues[COMPUTE_IDX] = device.getQueue(indices.compute, 0);
            queues[TRANSFER_IDX] = device.getQueue(indices.transfer, 0);
            queues[PRESENT_IDX] = device.getQueue(indices.present, 0);
        }

        void self_destruct()
        {
            device.destroy();
        }

        const vk::Device& get() const
        {
            return device;
        }

    private:
        vk::Device device;
    };
}// namespace ve
