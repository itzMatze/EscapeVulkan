#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"

namespace ve
{
    class Synchronization
    {
    public:
        Synchronization(const vk::Device logical_device) : device(logical_device)
        {
        }

        ~Synchronization()
        {
            for (auto& s: semaphores)
            {
                device.destroy(s);
            }
            for (auto& f: fences)
            {
                device.destroyFence(f);
            }
        }

        uint32_t add_semaphore()
        {
            vk::SemaphoreCreateInfo sci{};
            sci.sType = vk::StructureType::eSemaphoreCreateInfo;
            semaphores.push_back(device.createSemaphore(sci));
            return semaphores.size() - 1;
        }

        uint32_t add_fence()
        {
            vk::FenceCreateInfo fci{};
            fci.sType = vk::StructureType::eFenceCreateInfo;
            fences.push_back(device.createFence(fci));
            return fences.size() - 1;
        }

        const vk::Semaphore& get_semaphore(uint32_t idx) const
        {
            return semaphores[idx];
        }

        const vk::Fence& get_fence(uint32_t idx) const
        {
            return fences[idx];
        }

        void wait_for_fence(uint32_t idx) const
        {
            VE_CHECK(device.waitForFences(fences[idx], 1, uint64_t(-1)), "Failed to wait for fence!");
        }

        void reset_fence(uint32_t idx) const
        {
            device.resetFences(fences[idx]);
        }

    private:
        const vk::Device device;
        std::vector<vk::Semaphore> semaphores;
        std::vector<vk::Fence> fences;
    };
}// namespace ve
