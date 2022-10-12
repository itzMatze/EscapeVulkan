#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.hpp>

#include "vk/CommandPool.hpp"
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

        const vk::CommandBuffer& begin(const vk::CommandBuffer& cb) const
        {
            vk::CommandBufferBeginInfo cbbi{};
            cbbi.sType = vk::StructureType::eCommandBufferBeginInfo;
            cbbi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            cb.begin(cbbi);
            return cb;
        }

        void submit_graphics(const vk::CommandBuffer& cb, bool wait_idle) const
        {
            submit(cb, vmc.get_graphics_queue(), wait_idle);
        }

        void submit_compute(const vk::CommandBuffer& cb, bool wait_idle) const
        {
            submit(cb, vmc.get_compute_queue(), wait_idle);
        }

        void submit_transfer(const vk::CommandBuffer& cb, bool wait_idle) const
        {
            submit(cb, vmc.get_transfer_queue(), wait_idle);
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

    private:
        void submit(const vk::CommandBuffer& cb, const vk::Queue& queue, bool wait_idle) const
        {
            cb.end();
            vk::SubmitInfo submit_info{};
            submit_info.sType = vk::StructureType::eSubmitInfo;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cb;
            queue.submit(submit_info);
            if (wait_idle) queue.waitIdle();
            cb.reset();
        }
    };
}// namespace ve
