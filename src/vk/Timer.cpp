#include "vk/Timer.hpp"

namespace ve
{
    DeviceTimer::DeviceTimer(const VulkanMainContext& vmc) : vmc(vmc)
    {
        vk::QueryPoolCreateInfo qpci{};
        qpci.sType = vk::StructureType::eQueryPoolCreateInfo;
        qpci.queryType = vk::QueryType::eTimestamp;
        qpci.queryCount = TIMER_COUNT * 2;
        qp = vmc.logical_device.get().createQueryPool(qpci);
        vk::PhysicalDeviceProperties pdp = vmc.physical_device.get().getProperties();
        timestamp_period = pdp.limits.timestampPeriod;
    }

    void DeviceTimer::self_destruct()
    {
        vmc.logical_device.get().destroyQueryPool(qp);
    }

    void DeviceTimer::reset_all(vk::CommandBuffer& cb)
    {
        cb.resetQueryPool(qp, 0, TIMER_COUNT * 2);
    }

    void DeviceTimer::start(vk::CommandBuffer& cb, TimerNames t, vk::PipelineStageFlagBits stage)
    {
        cb.writeTimestamp(stage, qp, t * 2);
    }

    void DeviceTimer::stop(vk::CommandBuffer& cb, TimerNames t, vk::PipelineStageFlagBits stage)
    {
        cb.writeTimestamp(stage, qp, t * 2 + 1);
    }
} // namespace ve
