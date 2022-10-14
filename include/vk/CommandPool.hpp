#pragma once

#include <vulkan/vulkan.hpp>

namespace ve
{
    class CommandPool
    {
    public:
        CommandPool(const vk::Device& logical_device, uint32_t queue_family_idx);
        std::vector<vk::CommandBuffer> create_command_buffers(uint32_t count);
        void self_destruct();

    private:
        const vk::Device device;
        vk::CommandPool command_pool;
    };
}// namespace ve