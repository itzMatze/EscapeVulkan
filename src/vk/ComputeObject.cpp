#include "vk/ComputeObject.hpp"

namespace ve
{
    ComputeObject::ComputeObject(const VulkanMainContext& vmc) : dsh(vmc), vmc(vmc), pipeline(vmc)
    {}

    void ComputeObject::self_destruct()
    {
        pipeline.self_destruct();
        dsh.self_destruct();
    }

    void ComputeObject::construct(const ShaderInfo& shader_name, uint32_t set_count)
    {
        for (uint32_t i = 0; i < set_count; ++i) dsh.new_set();
        dsh.construct();
        pipeline.construct(dsh.get_layouts()[0], shader_name);
    }

    void ComputeObject::compute(vk::CommandBuffer& cb, DrawInfo& di)
    {
        cb.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline.get());
        cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeline.get_layout(), 0, dsh.get_sets()[di.current_frame], {});
        // TODO(Matze): dispatch shader
    }
} // namespace ve
