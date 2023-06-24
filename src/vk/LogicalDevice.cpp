#include "vk/LogicalDevice.hpp"

#include <optional>
#include <set>

#include "vk/common.hpp"
#include "ve_log.hpp"
#include "vk/PhysicalDevice.hpp"
#include "vk/Synchronization.hpp"

namespace ve
{
    LogicalDevice::LogicalDevice(const PhysicalDevice& p_device, const QueueFamilyIndices& queue_family_indices, std::unordered_map<QueueIndex, vk::Queue>& queues)
    {
        std::vector<vk::DeviceQueueCreateInfo> qci_s;
        std::set<uint32_t> unique_queue_families = {queue_family_indices.graphics, queue_family_indices.compute, queue_family_indices.transfer, queue_family_indices.present};
        float queue_prio = 1.0f;
        for (uint32_t queue_family : unique_queue_families)
        {
            vk::DeviceQueueCreateInfo qci{};
            qci.sType = vk::StructureType::eDeviceQueueCreateInfo;
            qci.queueFamilyIndex = queue_family;
            qci.queueCount = 1;
            qci.pQueuePriorities = &queue_prio;
            qci_s.push_back(qci);
        }

        vk::PhysicalDeviceRayQueryFeaturesKHR rq_features;
        rq_features.rayQuery = VK_TRUE;

        vk::PhysicalDeviceAccelerationStructureFeaturesKHR as_features;
        as_features.pNext = &rq_features;
        as_features.accelerationStructure = VK_TRUE;

        vk::PhysicalDeviceVulkan12Features device_features_12;
        device_features_12.pNext = &as_features;
        device_features_12.bufferDeviceAddress = VK_TRUE;

        vk::PhysicalDeviceVulkan13Features device_features_13;
        device_features_13.pNext = &device_features_12;
        device_features_13.synchronization2 = VK_TRUE;

        vk::PhysicalDeviceFeatures core_device_features{};
        core_device_features.samplerAnisotropy = VK_TRUE;
        core_device_features.sampleRateShading = VK_TRUE;
        core_device_features.fillModeNonSolid = VK_TRUE;
        core_device_features.wideLines = VK_TRUE;

        vk::PhysicalDeviceFeatures2 device_features;
        device_features.pNext = &device_features_13;
        device_features.features = core_device_features;

        vk::DeviceCreateInfo dci{};
        dci.sType = vk::StructureType::eDeviceCreateInfo;
        dci.pNext = &device_features;
        dci.queueCreateInfoCount = qci_s.size();
        dci.pQueueCreateInfos = qci_s.data();
        dci.enabledExtensionCount = p_device.get_extensions().size();
        dci.ppEnabledExtensionNames = p_device.get_extensions().data();

        device = p_device.get().createDevice(dci);
        queues.emplace(QueueIndex::Graphics, device.getQueue(queue_family_indices.graphics, 0));
        queues.emplace(QueueIndex::Compute, device.getQueue(queue_family_indices.compute, 0));
        queues.emplace(QueueIndex::Transfer, device.getQueue(queue_family_indices.transfer, 0));
        queues.emplace(QueueIndex::Present, device.getQueue(queue_family_indices.present, 0));
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
    }

    void LogicalDevice::self_destruct()
    {
        device.destroy();
    }

    const vk::Device& LogicalDevice::get() const
    {
        return device;
    }
} // namespace ve
