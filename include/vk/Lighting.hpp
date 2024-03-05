#pragma once

#include <cstdint>
#include <vector>

#include "vk/Pipeline.hpp"
#include "vk/DescriptorSetHandler.hpp"

namespace ve
{
class VulkanMainContext;
class Storage;
class Swapchain;

class Lighting
{
public:
    Lighting(const VulkanMainContext& vmc, Storage& storage);
    void construct(uint32_t light_count, const Swapchain& swapchain);
    void self_destruct();
    void pre_pass(vk::CommandBuffer& cb, GameState& gs);
    void main_pass(vk::CommandBuffer& cb, GameState& gs);
private:
    const VulkanMainContext& vmc;
    Storage& storage;
    std::vector<uint32_t> restir_reservoir_buffers;
    Pipeline lighting_pipeline_0;
    Pipeline lighting_pipeline_1;
    DescriptorSetHandler lighting_dsh;

    void create_lighting_pipeline(uint32_t light_count, const Swapchain& swapchain);
    void create_lighting_descriptor_sets(vk::Extent2D swapchain_extent);
};
} // namespace ve

