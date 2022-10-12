#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"
#include "vk/Swapchain.hpp"

namespace ve
{
    class CommandPool
    {
    public:
        CommandPool(const vk::Device& logical_device, uint32_t queue_family_idx) : device(logical_device)
        {
            vk::CommandPoolCreateInfo cpci{};
            cpci.sType = vk::StructureType::eCommandPoolCreateInfo;
            cpci.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            cpci.queueFamilyIndex = queue_family_idx;
            command_pool = device.createCommandPool(cpci);
        }

        std::vector<vk::CommandBuffer> create_command_buffers(uint32_t count)
        {
            vk::CommandBufferAllocateInfo cbai{};
            cbai.sType = vk::StructureType::eCommandBufferAllocateInfo;
            cbai.commandPool = command_pool;
            // secondary command buffers can be called from primary command buffers
            cbai.level = vk::CommandBufferLevel::ePrimary;
            cbai.commandBufferCount = count;
            return device.allocateCommandBuffers(cbai);
        }

        void self_destruct()
        {
            device.destroyCommandPool(command_pool);
        }
        
    private:
        const vk::Device device;
        vk::CommandPool command_pool;
    };
}// namespace ve