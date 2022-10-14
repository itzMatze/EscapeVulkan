#include "vk/LogicalDevice.hpp"

#include <optional>
#include <set>
#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"
#include "vk/PhysicalDevice.hpp"
#include "vk/Synchronization.hpp"

namespace ve
{
    LogicalDevice::LogicalDevice(const PhysicalDevice& p_device, QueueFamilyIndices& indices, std::unordered_map<QueueIndex, vk::Queue>& queues)
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
        device_features.samplerAnisotropy = VK_TRUE;
        vk::DeviceCreateInfo dci{};
        dci.sType = vk::StructureType::eDeviceCreateInfo;
        dci.queueCreateInfoCount = qci_s.size();
        dci.pQueueCreateInfos = qci_s.data();
        dci.enabledExtensionCount = p_device.get_extensions().size();
        dci.ppEnabledExtensionNames = p_device.get_extensions().data();
        dci.pEnabledFeatures = &device_features;

        device = p_device.get().createDevice(dci);
        queues.emplace(QueueIndex::Graphics, device.getQueue(indices.graphics, 0));
        queues.emplace(QueueIndex::Compute, device.getQueue(indices.compute, 0));
        queues.emplace(QueueIndex::Transfer, device.getQueue(indices.transfer, 0));
        queues.emplace(QueueIndex::Present, device.getQueue(indices.present, 0));
    }

    void LogicalDevice::self_destruct()
    {
        device.destroy();
    }

    const vk::Device& LogicalDevice::get() const
    {
        return device;
    }
}// namespace ve
