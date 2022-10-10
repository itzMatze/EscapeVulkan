#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/Buffer.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class DescriptorSetHandler
    {
    public:
        DescriptorSetHandler(const VulkanMainContext& vmc) : vmc(vmc)
        {}

        template<class T>
        std::vector<ve::Buffer> add_uniform_buffer(uint32_t copies, const std::vector<T>& data, const std::vector<uint32_t>& queue_family_indices)
        {
            vk::DescriptorSetLayoutBinding uniform_buffer{};
            uniform_buffer.binding = 0;
            uniform_buffer.descriptorType = vk::DescriptorType::eUniformBuffer;
            uniform_buffer.descriptorCount = 1;
            uniform_buffer.stageFlags = vk::ShaderStageFlagBits::eAll;
            uniform_buffer.pImmutableSamplers = nullptr;

            layout_bindings.push_back(uniform_buffer);

            std::vector<ve::Buffer> uniform_buffers;
            for (uint32_t i = 0; i < copies; ++i)
            {
                uniform_buffers.push_back(Buffer(vmc, data, vk::BufferUsageFlagBits::eUniformBuffer, queue_family_indices));

                vk::DescriptorBufferInfo uniform_dbi{};
                uniform_dbi.buffer = uniform_buffers[i].get();
                uniform_dbi.offset = 0;
                uniform_dbi.range = uniform_buffers[i].get_byte_size();

                uniform_buffer_infos.push_back(uniform_dbi);
            }

            return uniform_buffers;
        }

        void add_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, const ve::Buffer& buffer)
        {
            vk::DescriptorSetLayoutBinding dslb{};
            dslb.binding = binding;
            dslb.descriptorType = type;
            dslb.descriptorCount = 1;
            dslb.stageFlags = stages;
            dslb.pImmutableSamplers = nullptr;

            layout_bindings.push_back(dslb);

            vk::DescriptorBufferInfo dbi{};
            dbi.buffer = buffer.get();
            dbi.offset = 0;
            dbi.range = buffer.get_byte_size();

            buffer_infos.push_back(dbi);
        }

        void construct()
        {
            std::vector<vk::DescriptorPoolSize> pool_sizes;
            pool_sizes.push_back(vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, uniform_buffer_infos.size()));
            for (const auto& dslb: layout_bindings)
            {
                vk::DescriptorPoolSize dps{};
                dps.type = dslb.descriptorType;
                dps.descriptorCount = 1;

                pool_sizes.push_back(dps);
            }

            vk::DescriptorSetLayoutCreateInfo dslci{};
            dslci.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
            dslci.bindingCount = layout_bindings.size();
            dslci.pBindings = layout_bindings.data();
            for (uint32_t i = 0; i < uniform_buffer_infos.size(); ++i)
            {
                layouts.push_back(vmc.logical_device.get().createDescriptorSetLayout(dslci));
            }

            vk::DescriptorPoolCreateInfo dpci{};
            dpci.sType = vk::StructureType::eDescriptorPoolCreateInfo;
            dpci.poolSizeCount = pool_sizes.size();
            dpci.pPoolSizes = pool_sizes.data();
            dpci.maxSets = uniform_buffer_infos.size();

            pool = vmc.logical_device.get().createDescriptorPool(dpci);

            vk::DescriptorSetAllocateInfo dsai{};
            dsai.sType = vk::StructureType::eDescriptorSetAllocateInfo;
            dsai.descriptorPool = pool;
            dsai.descriptorSetCount = layouts.size();
            dsai.pSetLayouts = layouts.data();

            sets = vmc.logical_device.get().allocateDescriptorSets(dsai);

            std::vector<vk::WriteDescriptorSet> wds_s;
            for (uint32_t i = 0; i < sets.size(); ++i)
            {
                vk::WriteDescriptorSet uniform_wds{};
                uniform_wds.sType = vk::StructureType::eWriteDescriptorSet;
                uniform_wds.dstSet = sets[i];
                uniform_wds.dstBinding = 0;
                uniform_wds.dstArrayElement = 0;

                uniform_wds.descriptorType = vk::DescriptorType::eUniformBuffer;
                uniform_wds.descriptorCount = 1;
                uniform_wds.pBufferInfo = &uniform_buffer_infos[i];
                uniform_wds.pImageInfo = nullptr;
                uniform_wds.pTexelBufferView = nullptr;
                wds_s.push_back(uniform_wds);

                for (uint32_t j = 1; j < layout_bindings.size(); ++j)
                {
                    vk::WriteDescriptorSet wds{};
                    wds.sType = vk::StructureType::eWriteDescriptorSet;
                    wds.dstSet = sets[i];
                    wds.dstBinding = layout_bindings[j].binding;
                    wds.dstArrayElement = 0;

                    wds.descriptorType = layout_bindings[j].descriptorType;
                    wds.descriptorCount = 1;
                    wds.pBufferInfo = &buffer_infos[j];
                    wds.pImageInfo = nullptr;
                    wds.pTexelBufferView = nullptr;

                    wds_s.push_back(wds);
                }
            }
            vmc.logical_device.get().updateDescriptorSets(wds_s, {});
        }

        void self_destruct()
        {
            vmc.logical_device.get().destroyDescriptorPool(pool);
            for (auto& dsl: layouts)
            {
                vmc.logical_device.get().destroyDescriptorSetLayout(dsl);
            }
        }

        const std::vector<vk::DescriptorSetLayout>& get_layouts() const
        {
            return layouts;
        }

        const std::vector<vk::DescriptorSet>& get_sets() const
        {
            return sets;
        }

    private:
        const VulkanMainContext& vmc;
        std::vector<vk::DescriptorBufferInfo> uniform_buffer_infos;
        std::vector<vk::DescriptorBufferInfo> buffer_infos;
        std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
        std::vector<vk::DescriptorSetLayout> layouts;
        vk::DescriptorPool pool;
        std::vector<vk::DescriptorSet> sets;
    };
}// namespace ve
