#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.hpp>

#include "vk/Buffer.hpp"
#include "vk/CommandPool.hpp"
#include "vk/Image.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class VulkanCommandContext
    {
    public:
        VulkanCommandContext(VulkanMainContext& vmc) : vmc(vmc), sync(vmc.logical_device.get())
        {
            command_pools.push_back(CommandPool(vmc.logical_device.get(), vmc.queues_family_indices.graphics));
            command_pools.push_back(CommandPool(vmc.logical_device.get(), vmc.queues_family_indices.compute));
            command_pools.push_back(CommandPool(vmc.logical_device.get(), vmc.queues_family_indices.transfer));
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Created VulkanCommandContext\n");
        }

        void add_graphics_buffers(uint32_t count)
        {
            auto tmp = command_pools[0].create_command_buffers(count);
            graphics_cb.insert(graphics_cb.end(), tmp.begin(), tmp.end());
        }

        void add_compute_buffers(uint32_t count)
        {
            auto tmp = command_pools[1].create_command_buffers(count);
            compute_cb.insert(compute_cb.end(), tmp.begin(), tmp.end());
        }

        void add_transfer_buffers(uint32_t count)
        {
            auto tmp = command_pools[2].create_command_buffers(count);
            transfer_cb.insert(transfer_cb.end(), tmp.begin(), tmp.end());
        }

        vk::CommandBuffer& begin(vk::CommandBuffer& cb)
        {
            vk::CommandBufferBeginInfo cbbi{};
            cbbi.sType = vk::StructureType::eCommandBufferBeginInfo;
            cbbi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            cb.begin(cbbi);
            return cb;
        }

        void self_destruct()
        {
            sync.wait_idle();
            for (auto& command_pool: command_pools)
            {
                command_pool.self_destruct();
            }
            command_pools.clear();
            sync.self_destruct();
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Destroyed VulkanCommandContext\n");
        }

        const VulkanMainContext& vmc;
        Synchronization sync;
        std::vector<CommandPool> command_pools;
        std::vector<vk::CommandBuffer> graphics_cb;
        std::vector<vk::CommandBuffer> compute_cb;
        std::vector<vk::CommandBuffer> transfer_cb;
    };
}// namespace ve
