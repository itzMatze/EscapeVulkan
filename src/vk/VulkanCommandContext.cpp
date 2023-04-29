#include "vk/VulkanCommandContext.hpp"

#include "ve_log.hpp"

namespace ve
{
        VulkanCommandContext::VulkanCommandContext(VulkanMainContext& vmc) : vmc(vmc)
        {
            command_pools.push_back(CommandPool(vmc.logical_device.get(), vmc.queue_family_indices.graphics));
            command_pools.push_back(CommandPool(vmc.logical_device.get(), vmc.queue_family_indices.compute));
            command_pools.push_back(CommandPool(vmc.logical_device.get(), vmc.queue_family_indices.transfer));
            spdlog::info("Created VulkanCommandContext");
        }

        void VulkanCommandContext::add_graphics_buffers(uint32_t count)
        {
            auto tmp = command_pools[0].create_command_buffers(count);
            graphics_cb.insert(graphics_cb.end(), tmp.begin(), tmp.end());
        }

        void VulkanCommandContext::add_compute_buffers(uint32_t count)
        {
            auto tmp = command_pools[1].create_command_buffers(count);
            compute_cb.insert(compute_cb.end(), tmp.begin(), tmp.end());
        }

        void VulkanCommandContext::add_transfer_buffers(uint32_t count)
        {
            auto tmp = command_pools[2].create_command_buffers(count);
            transfer_cb.insert(transfer_cb.end(), tmp.begin(), tmp.end());
        }

        vk::CommandBuffer& VulkanCommandContext::begin(vk::CommandBuffer& cb)
        {
            vk::CommandBufferBeginInfo cbbi{};
            cbbi.sType = vk::StructureType::eCommandBufferBeginInfo;
            cbbi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            cb.begin(cbbi);
            return cb;
        }

        void VulkanCommandContext::submit_graphics(const vk::CommandBuffer& cb, bool wait_idle) const
        {
            submit(cb, vmc.get_graphics_queue(), wait_idle);
        }

        void VulkanCommandContext::submit_compute(const vk::CommandBuffer& cb, bool wait_idle) const
        {
            submit(cb, vmc.get_compute_queue(), wait_idle);
        }

        void VulkanCommandContext::submit_transfer(const vk::CommandBuffer& cb, bool wait_idle) const
        {
            submit(cb, vmc.get_transfer_queue(), wait_idle);
        }

        void VulkanCommandContext::self_destruct()
        {
            for (auto& command_pool : command_pools) command_pool.self_destruct();
            command_pools.clear();
            spdlog::info("Destroyed VulkanCommandContext");
        }

        void VulkanCommandContext::submit(const vk::CommandBuffer& cb, const vk::Queue& queue, bool wait_idle) const
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
} // namespace ve
