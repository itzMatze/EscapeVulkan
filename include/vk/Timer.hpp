#pragma once

#include <vulkan/vulkan.hpp>
#include <chrono>

#include "ve_log.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    template<class Precision = float>
    class HostTimer
    {
    public:
        HostTimer() : t(std::chrono::high_resolution_clock::now())
        {}

        template<class Period = std::milli>
        inline Precision restart()
        {
            double elapsed_time = elapsed<Period>();
            t = std::chrono::high_resolution_clock::now();
            return elapsed_time;
        }

        template<class Period = std::milli>
        inline Precision elapsed() const
        {
            return std::chrono::duration<Precision, Period>(std::chrono::high_resolution_clock::now() - t).count();
        }

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> t;
    };

    class DeviceTimer
    {
    public:
        enum TimerNames {
            ALL_RENDERING = 0,
            APP_RENDERING = 1,
            UI_RENDERING = 2,
            TIMER_COUNT
        };

        DeviceTimer(const VulkanMainContext& vmc);
        void self_destruct();
        void reset_all(vk::CommandBuffer& cb);
        void start(vk::CommandBuffer& cb, TimerNames t, vk::PipelineStageFlagBits stage);
        void stop(vk::CommandBuffer& cb, TimerNames t, vk::PipelineStageFlagBits stage);

        template<class Precision = std::milli>
        double inline get_result_by_idx(uint32_t i)
        {
            VE_ASSERT(i < TIMER_COUNT, "Trying to access timer index that does not exist in TimerNames");
            return get_result<Precision>(i);
        }

        template<class Precision = std::milli>
        double inline get_result(TimerNames t)
        {
            return get_result<Precision>(t);
        }

    private:
        template<class Precision>
        double inline get_result(uint32_t i)
        {
            std::array<uint64_t, 2> results; 
            vk::Result result = vmc.logical_device.get().getQueryPoolResults(qp, i * 2, 2, results.size() * sizeof(uint64_t), results.data(), sizeof(uint64_t), vk::QueryResultFlagBits::e64);
            return (result == vk::Result::eSuccess) ? (double(timestamp_period * (results[1] - results[0])) / double(std::ratio_divide<std::nano, Precision>::den)) : -1.0;
        }

        const VulkanMainContext& vmc;
        vk::QueryPool qp;
        float timestamp_period;
    };
} // namespace ve
