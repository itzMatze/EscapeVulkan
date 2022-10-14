#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/Synchronization.hpp"
#include "vk/CommandPool.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class VulkanCommandContext
    {
    public:
        VulkanCommandContext(VulkanMainContext& vmc);
        void add_graphics_buffers(uint32_t count);
        void add_compute_buffers(uint32_t count);
        void add_transfer_buffers(uint32_t count);
        const vk::CommandBuffer& begin(const vk::CommandBuffer& cb) const;
        void submit_graphics(const vk::CommandBuffer& cb, bool wait_idle) const;
        void submit_compute(const vk::CommandBuffer& cb, bool wait_idle) const;
        void submit_transfer(const vk::CommandBuffer& cb, bool wait_idle) const;
        void self_destruct();

        const VulkanMainContext& vmc;
        Synchronization sync;
        std::vector<CommandPool> command_pools;
        std::vector<vk::CommandBuffer> graphics_cb;
        std::vector<vk::CommandBuffer> compute_cb;
        std::vector<vk::CommandBuffer> transfer_cb;

    private:
        void submit(const vk::CommandBuffer& cb, const vk::Queue& queue, bool wait_idle) const;
    };
}// namespace ve
