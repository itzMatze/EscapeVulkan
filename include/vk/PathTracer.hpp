#pragma once

#include <vk/common.hpp>
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "Storage.hpp"
#include "vk/Pipeline.hpp"

namespace ve
{
    struct BottomLevelAccelerationStructure {
        vk::AccelerationStructureKHR handle;
        uint64_t deviceAddress = 0;
        uint32_t buffer;
        uint32_t scratch_buffer;
    };

    struct TopLevelAccelerationStructure {
        vk::AccelerationStructureKHR handle;
        uint64_t deviceAddress = 0;
        uint32_t buffer;
        uint32_t scratch_buffer;
        bool is_built = false;
    };

    class PathTracer
    {
    public:
        PathTracer(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void self_destruct();
        uint32_t add_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, uint32_t index_offset, uint32_t num_indices);
        uint32_t add_instance(uint32_t blas_idx, const glm::mat4& M, uint32_t custom_index);
        void update_instance(uint32_t instance_idx, const glm::mat4& M);
        void create_tlas(vk::CommandBuffer& cb, uint32_t idx);
        void update_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, uint32_t index_offset, uint32_t num_indices, uint32_t blas_idx);

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        std::array<vk::WriteDescriptorSetAccelerationStructureKHR, 2> wdsas;
        std::vector<BottomLevelAccelerationStructure> bottomLevelAS;
        std::vector<vk::AccelerationStructureInstanceKHR> instances;
        std::array<TopLevelAccelerationStructure, 2> topLevelAS;
        std::array<uint32_t, 2> instances_buffer;

        BottomLevelAccelerationStructure create_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, uint32_t index_offset, uint32_t num_indices);
    };
} // namespace ve
