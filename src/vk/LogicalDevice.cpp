#include "vk/LogicalDevice.hpp"

#include <optional>
#include <set>
#include <vulkan/vulkan.hpp>

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

        vk::PhysicalDeviceFeatures device_features{};
        device_features.samplerAnisotropy = VK_TRUE;
        device_features.sampleRateShading = VK_TRUE;
        device_features.fillModeNonSolid = VK_TRUE;
        vk::DeviceCreateInfo dci{};
        dci.sType = vk::StructureType::eDeviceCreateInfo;
        dci.queueCreateInfoCount = qci_s.size();
        dci.pQueueCreateInfos = qci_s.data();
        dci.enabledExtensionCount = p_device.get_extensions().size();
        dci.ppEnabledExtensionNames = p_device.get_extensions().data();
        dci.pEnabledFeatures = &device_features;

        device = p_device.get().createDevice(dci);
        queues.emplace(QueueIndex::Graphics, device.getQueue(queue_family_indices.graphics, 0));
        queues.emplace(QueueIndex::Compute, device.getQueue(queue_family_indices.compute, 0));
        queues.emplace(QueueIndex::Transfer, device.getQueue(queue_family_indices.transfer, 0));
        queues.emplace(QueueIndex::Present, device.getQueue(queue_family_indices.present, 0));
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
