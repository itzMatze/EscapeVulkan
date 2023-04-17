#include "vk/CommandPool.hpp"

#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"

namespace ve
{
    CommandPool::CommandPool(const vk::Device& logical_device, uint32_t queue_family_idx) : device(logical_device)
    {
        vk::CommandPoolCreateInfo cpci{};
        cpci.sType = vk::StructureType::eCommandPoolCreateInfo;
        cpci.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        cpci.queueFamilyIndex = queue_family_idx;
        command_pool = device.createCommandPool(cpci);
    }

    std::vector<vk::CommandBuffer> CommandPool::create_command_buffers(uint32_t count)
    {
        vk::CommandBufferAllocateInfo cbai{};
        cbai.sType = vk::StructureType::eCommandBufferAllocateInfo;
        cbai.commandPool = command_pool;
        // secondary command buffers can be called from primary command buffers
        cbai.level = vk::CommandBufferLevel::ePrimary;
        cbai.commandBufferCount = count;
        return device.allocateCommandBuffers(cbai);
    }

    void CommandPool::self_destruct()
    {
        device.destroyCommandPool(command_pool);
    }
} // namespace ve
