#include "vk/Synchronization.hpp"

#include "ve_log.hpp"

namespace ve
{
    Synchronization::Synchronization(const vk::Device& logical_device) : device(logical_device)
    {
        for (uint32_t i = 0; i < SEMAPHORE_COUNT; ++i)
        {
            vk::SemaphoreCreateInfo sci{};
            sci.sType = vk::StructureType::eSemaphoreCreateInfo;
            semaphores.push_back(device.createSemaphore(sci));
        }
        // all fences are created as signaled, if an unsignaled fence is needed use reset_fence
        for (uint32_t i = 0; i < FENCE_COUNT; ++i)
        {
            vk::FenceCreateInfo fci{};
            fci.sType = vk::StructureType::eFenceCreateInfo;
            fci.flags = vk::FenceCreateFlagBits::eSignaled;
            fences.push_back(device.createFence(fci));
        }
    }

    void Synchronization::self_destruct()
    {
        wait_idle();
        for (auto& s : semaphores) device.destroy(s);
        semaphores.clear();
        for (auto& f : fences) device.destroyFence(f);
        fences.clear();
    }

    const vk::Semaphore& Synchronization::get_semaphore(SemaphoreNames name) const
    {
        return semaphores[name];
    }

    const vk::Fence& Synchronization::get_fence(FenceNames name) const
    {
        return fences[name];
    }

    void Synchronization::wait_for_fence(FenceNames name) const
    {
        VE_CHECK(device.waitForFences(fences[name], 1, uint64_t(-1)), "Failed to wait for fence!");
    }

    void Synchronization::reset_fence(FenceNames name) const
    {
        device.resetFences(fences[name]);
    }

    void Synchronization::wait_idle() const
    {
        device.waitIdle();
    }
} // namespace ve
