#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class DescriptorSetHandler
    {
    public:
        DescriptorSetHandler(const VulkanMainContext& vmc) : vmc(vmc)
        {
            create_layout();
        }

        void create_layout()
        {
            vk::DescriptorSetLayoutBinding uniform_buffer{};
            uniform_buffer.binding = 0;
            uniform_buffer.descriptorType = vk::DescriptorType::eUniformBuffer;
            uniform_buffer.descriptorCount = 1;
            uniform_buffer.stageFlags = vk::ShaderStageFlagBits::eVertex;
            uniform_buffer.pImmutableSamplers = nullptr;

            vk::DescriptorSetLayoutCreateInfo dslci{};
            dslci.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
            dslci.bindingCount = 1;
            dslci.pBindings = &uniform_buffer;
            descriptor_set_layouts.push_back(vmc.logical_device.get().createDescriptorSetLayout(dslci));
            descriptor_set_layouts.push_back(vmc.logical_device.get().createDescriptorSetLayout(dslci));
        }

        void create_sets(auto uniform_buffers_range)
        {
            vk::DescriptorPoolSize dps{};
            dps.type = vk::DescriptorType::eUniformBuffer;
            dps.descriptorCount = 2;

            vk::DescriptorPoolCreateInfo dpci{};
            dpci.sType = vk::StructureType::eDescriptorPoolCreateInfo;
            dpci.poolSizeCount = 1;
            dpci.pPoolSizes = &dps;
            dpci.maxSets = 2;

            descriptor_pools.push_back(vmc.logical_device.get().createDescriptorPool(dpci));

            vk::DescriptorSetAllocateInfo dsai{};
            dsai.sType = vk::StructureType::eDescriptorSetAllocateInfo;
            dsai.descriptorPool = descriptor_pools[0];
            dsai.descriptorSetCount = 2;
            dsai.pSetLayouts = descriptor_set_layouts.data();

            descriptor_sets = vmc.logical_device.get().allocateDescriptorSets(dsai);

            for (uint32_t i = 0; i < 2; ++i)
            {
                vk::DescriptorBufferInfo dbi{};
                dbi.buffer = uniform_buffers_range.first->second.get();
                std::advance(uniform_buffers_range.first, 1);
                dbi.offset = 0;
                dbi.range = uniform_buffers_range.first->second.get_byte_size();

                vk::WriteDescriptorSet wds{};
                wds.sType = vk::StructureType::eWriteDescriptorSet;
                wds.dstSet = descriptor_sets[i];
                wds.dstBinding = 0;
                wds.dstArrayElement = 0;

                wds.descriptorType = vk::DescriptorType::eUniformBuffer;
                wds.descriptorCount = 1;
                wds.pBufferInfo = &dbi;
                wds.pImageInfo = nullptr;
                wds.pTexelBufferView = nullptr;

                vmc.logical_device.get().updateDescriptorSets(wds, {});
            }
        }

        void self_destruct()
        {
            for (auto& dp: descriptor_pools)
            {
                vmc.logical_device.get().destroyDescriptorPool(dp);
            }
            for (auto& dsl: descriptor_set_layouts)
            {
                vmc.logical_device.get().destroyDescriptorSetLayout(dsl);
            }
        }

        const std::vector<vk::DescriptorSetLayout>& get_layouts() const
        {
            return descriptor_set_layouts;
        }

        const std::vector<vk::DescriptorSet>& get_sets() const
        {
            return descriptor_sets;
        }

    private:
        const VulkanMainContext& vmc;
        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
        std::vector<vk::DescriptorPool> descriptor_pools;
        std::vector<vk::DescriptorSet> descriptor_sets;
    };
}// namespace ve
