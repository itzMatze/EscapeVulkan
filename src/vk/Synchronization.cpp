#include "vk/Synchronization.hpp"

#include "ve_log.hpp"

namespace ve
{
    Synchronization::Synchronization(const vk::Device& logical_device) : device(logical_device)
    {}

    void Synchronization::self_destruct()
    {
        for (auto& s : semaphores)
        {
            device.destroy(s);
        }
        semaphores.clear();
        for (auto& f : fences)
        {
            device.destroyFence(f);
        }
        fences.clear();
    }

    uint32_t Synchronization::add_semaphore()
    {
        vk::SemaphoreCreateInfo sci{};
        sci.sType = vk::StructureType::eSemaphoreCreateInfo;
        semaphores.push_back(device.createSemaphore(sci));
        return semaphores.size() - 1;
    }

    uint32_t Synchronization::add_fence(bool signaled)
    {
        vk::FenceCreateInfo fci{};
        fci.sType = vk::StructureType::eFenceCreateInfo;
        if (signaled) fci.flags = vk::FenceCreateFlagBits::eSignaled;
        fences.push_back(device.createFence(fci));
        return fences.size() - 1;
    }

    const vk::Semaphore& Synchronization::get_semaphore(uint32_t idx) const
    {
        return semaphores[idx];
    }

    const vk::Fence& Synchronization::get_fence(uint32_t idx) const
    {
        return fences[idx];
    }

    void Synchronization::wait_for_fence(uint32_t idx) const
    {
        VE_CHECK(device.waitForFences(fences[idx], 1, uint64_t(-1)), "Failed to wait for fence!");
    }

    void Synchronization::reset_fence(uint32_t idx) const
    {
        device.resetFences(fences[idx]);
    }

    void Synchronization::wait_idle() const
    {
        device.waitIdle();
    }
} // namespace ve
