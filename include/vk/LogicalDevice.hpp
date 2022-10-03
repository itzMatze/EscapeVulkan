#pragma once

#include <optional>
#include <set>
#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"
#include "vk/PhysicalDevice.hpp"
#include "vk/Synchronization.hpp"

namespace ve
{
    class LogicalDevice
    {
    public:
        LogicalDevice(const PhysicalDevice& p_device)
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "logical device +\n");
            QueueFamilyIndices indices = p_device.get_queue_families();

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

            VE_LOG_CONSOLE(VE_INFO, "Creating logical device\n");
            device = p_device.get().createDevice(dci);
            queues.resize(4);
            queues_indices.graphics = 0;
            queues[queues_indices.graphics] = device.getQueue(indices.graphics, 0);
            queues_indices.compute = 1;
            queues[queues_indices.compute] = device.getQueue(indices.compute, 0);
            queues_indices.transfer = 2;
            queues[queues_indices.transfer] = device.getQueue(indices.transfer, 0);
            queues_indices.present = 3;
            queues[queues_indices.present] = device.getQueue(indices.present, 0);
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "logical device +++\n");
        }

        ~LogicalDevice()
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "logical device -\n");
            VE_LOG_CONSOLE(VE_INFO, "Destroying logical device\n");
            device.destroy();
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "logical device ---\n");
        }

        vk::Device get() const
        {
            return device;
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

        void submit_graphics(const ve::Synchronization& sync, glm::vec3 sync_indices, vk::PipelineStageFlags wait_stage, vk::CommandBuffer command_buffer, vk::SwapchainKHR swapchain, uint32_t image_idx)
        {
            vk::SubmitInfo si{};
            si.sType = vk::StructureType::eSubmitInfo;
            si.waitSemaphoreCount = 1;
            si.pWaitSemaphores = &sync.get_semaphore(sync_indices.x);
            si.pWaitDstStageMask = &wait_stage;
            si.commandBufferCount = 1;
            si.pCommandBuffers = &command_buffer;
            si.signalSemaphoreCount = 1;
            si.pSignalSemaphores = &sync.get_semaphore(sync_indices.y);
            queues[queues_indices.graphics].submit(si, sync.get_fence(sync_indices.z));
            sync.wait_for_fence(sync_indices.z);
            sync.reset_fence(sync_indices.z);

            vk::PresentInfoKHR present_info{};
            present_info.sType = vk::StructureType::ePresentInfoKHR;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &sync.get_semaphore(sync_indices.y);
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &swapchain;
            present_info.pImageIndices = &image_idx;
            present_info.pResults = nullptr;
            VE_CHECK(queues[queues_indices.present].presentKHR(present_info), "Failed to present image!");
        }

    private:
        QueueFamilyIndices queues_indices;
        std::vector<vk::Queue> queues;
        vk::Device device;
    };
}// namespace ve
