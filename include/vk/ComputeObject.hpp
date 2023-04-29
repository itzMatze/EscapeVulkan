#pragma once

#include "vk/Pipeline.hpp"
#include "vk/common.hpp"

namespace ve
{
    class ComputeObject
    {
    public:
        ComputeObject(const VulkanMainContext& vmc);
        void self_destruct();
        void construct(const ShaderInfo& shader_name, uint32_t set_count);
        void compute(vk::CommandBuffer& cb, DrawInfo& di);

        DescriptorSetHandler dsh;

    private:
        const VulkanMainContext& vmc;
        Pipeline pipeline;
    };
} // namespace ve
