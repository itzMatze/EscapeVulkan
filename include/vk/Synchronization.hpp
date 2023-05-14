#pragma once

#include <vulkan/vulkan.hpp>

namespace ve
{
    class Synchronization
    {
    public:
        enum SemaphoreNames
        {
            S_IMAGE_AVAILABLE = 0,
            S_RENDER_FINISHED = 1,
            S_FIREFLY_MOVE_TUNNEL_ADVANCE_FINISHED = 2,
            SEMAPHORE_COUNT
        };

        enum FenceNames
        {
            F_RENDER_FINISHED = 0,
            FENCE_COUNT
        };

        Synchronization(const vk::Device& logical_device);
        void self_destruct();
        const vk::Semaphore& get_semaphore(SemaphoreNames name) const;
        const vk::Fence& get_fence(FenceNames name) const;
        void wait_for_fence(FenceNames name) const;
        void reset_fence(FenceNames name) const;
        void wait_idle() const;

    private:
        const vk::Device& device;
        std::vector<vk::Semaphore> semaphores;
        std::vector<vk::Fence> fences;
    };
} // namespace ve
