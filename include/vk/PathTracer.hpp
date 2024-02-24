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
        uint32_t scratch_buffer;;
        bool is_built = false;
    };

    struct TopLevelAccelerationStructure {
        vk::AccelerationStructureKHR handle;
        uint64_t deviceAddress = 0;
        uint32_t buffer;
        uint32_t scratch_buffer;
        bool is_built = false;
    };

    struct BLASBuildInfo {
        uint32_t vertex_buffer_id;
        uint32_t index_buffer_id;
        const std::vector<uint32_t> index_offsets;
        const std::vector<uint32_t> index_counts;
        vk::DeviceSize vertex_stride;
        uint32_t blas_idx;
    };

    class PathTracer
    {
    public:
        PathTracer(const VulkanMainContext& vmc, VulkanCommandContext& vcc, Storage& storage);
        void self_destruct();
        uint32_t add_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, const std::vector<uint32_t>& index_offsets, const std::vector<uint32_t>& index_counts, vk::DeviceSize vertex_stride);
        uint32_t add_instance(uint32_t blas_idx, const glm::mat4& M, uint32_t custom_index, uint32_t mask);
        void update_instance(uint32_t instance_idx, const glm::mat4& M);
        void create_tlas(vk::CommandBuffer& cb, uint32_t idx);
        void update_blas(uint32_t vertex_buffer_id, uint32_t index_buffer_id, const std::vector<uint32_t>& index_offsets, const std::vector<uint32_t>& index_counts, uint32_t blas_idx, uint32_t frame_idx, vk::DeviceSize vertex_stride);

    private:
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        Storage& storage;
        std::array<vk::WriteDescriptorSetAccelerationStructureKHR, 2> wdsas;
        std::array<std::vector<BottomLevelAccelerationStructure>, 2> bottomLevelAS;
        std::array<std::vector<BLASBuildInfo>, 2> bottomLevelAS_dirty_build_info;
        std::array<std::vector<vk::AccelerationStructureInstanceKHR>, 2> instances;
        std::array<TopLevelAccelerationStructure, 2> topLevelAS;
        std::array<uint32_t, 2> instances_buffer;

        void create_blas(vk::CommandBuffer& cb, uint32_t vertex_buffer_id, uint32_t index_buffer_id, const std::vector<uint32_t>& index_offsets, const std::vector<uint32_t>& index_counts, vk::DeviceSize vertex_stride, BottomLevelAccelerationStructure& blas);
    };
} // namespace ve
