#pragma once

#include <vulkan/vulkan.hpp>

namespace ve
{
    class Synchronization
    {
    public:
        Synchronization(const vk::Device& logical_device);
        void self_destruct();
        uint32_t add_semaphore();
        uint32_t add_fence();
        const vk::Semaphore& get_semaphore(uint32_t idx) const;
        const vk::Fence& get_fence(uint32_t idx) const;
        void wait_for_fence(uint32_t idx) const;
        void reset_fence(uint32_t idx) const;
        void wait_idle() const;

    private:
        const vk::Device& device;
        std::vector<vk::Semaphore> semaphores;
        std::vector<vk::Fence> fences;
    };
} // namespace ve
