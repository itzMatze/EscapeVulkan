#include "vk/DescriptorSetHandler.hpp"

namespace ve
{
    DescriptorSetHandler::DescriptorSetHandler(const VulkanMainContext& vmc) : vmc(vmc)
    {}

    uint32_t DescriptorSetHandler::new_set()
    {
        layout_bindings.push_back({});
        infos.push_back({});
        add_layout_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll);
        infos.back().insert(infos.back().end(), uniform_infos.begin(), uniform_infos.end());
        return (infos.size() - 1) * set_copies;
    }

    void DescriptorSetHandler::add_uniform_buffer(uint32_t copies, const std::vector<Buffer>& uniform_buffers)
    {
        set_copies = copies;

        for (uint32_t i = 0; i < copies; ++i)
        {
            vk::DescriptorBufferInfo uniform_dbi{};
            uniform_dbi.buffer = uniform_buffers[i].get();
            uniform_dbi.offset = 0;
            uniform_dbi.range = uniform_buffers[i].get_byte_size();

            uniform_infos.push_back(Binding(uniform_dbi, {}));
        }
    }

    void DescriptorSetHandler::add_buffer_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, const vk::Buffer& buffer, uint64_t byte_size)
    {
        add_layout_binding(binding, type, stages);

        vk::DescriptorBufferInfo dbi{};
        dbi.buffer = buffer;
        dbi.offset = 0;
        dbi.range = byte_size;

        infos.back().push_back(Binding(dbi, {}));
    }

    void DescriptorSetHandler::add_image_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, const Image& image)
    {
        add_layout_binding(binding, type, stages);

        vk::DescriptorImageInfo dii{};
        dii.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        dii.imageView = image.get_view();
        dii.sampler = image.get_sampler();

        infos.back().push_back(Binding({}, dii));
    }

    void DescriptorSetHandler::construct()
    {
        std::vector<vk::DescriptorPoolSize> pool_sizes;
        for (const auto& set_layout_bindings: layout_bindings)
        {
            for (const auto& dslb: set_layout_bindings)
            {
                vk::DescriptorPoolSize dps{};
                dps.type = dslb.descriptorType;
                dps.descriptorCount = dslb.descriptorCount;

                pool_sizes.push_back(dps);
            }

            vk::DescriptorSetLayoutCreateInfo dslci{};
            dslci.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
            dslci.bindingCount = set_layout_bindings.size();
            dslci.pBindings = set_layout_bindings.data();
            for (uint32_t i = 0; i < set_copies; ++i)
            {
                layouts.push_back(vmc.logical_device.get().createDescriptorSetLayout(dslci));
            }
        }

        vk::DescriptorPoolCreateInfo dpci{};
        dpci.sType = vk::StructureType::eDescriptorPoolCreateInfo;
        dpci.poolSizeCount = pool_sizes.size();
        dpci.pPoolSizes = pool_sizes.data();
        dpci.maxSets = infos.size() * set_copies;

        pool = vmc.logical_device.get().createDescriptorPool(dpci);

        vk::DescriptorSetAllocateInfo dsai{};
        dsai.sType = vk::StructureType::eDescriptorSetAllocateInfo;
        dsai.descriptorPool = pool;
        dsai.descriptorSetCount = layouts.size();
        dsai.pSetLayouts = layouts.data();

        sets = vmc.logical_device.get().allocateDescriptorSets(dsai);

        std::vector<vk::WriteDescriptorSet> wds_s;
        for (uint32_t i = 0; i < infos.size(); ++i)
        {
            for (uint32_t c = 0; c < set_copies; ++c)
            {
                vk::WriteDescriptorSet uniform_wds{};
                uniform_wds.sType = vk::StructureType::eWriteDescriptorSet;
                uniform_wds.dstSet = sets[(i * set_copies) + c];
                uniform_wds.dstBinding = 0;
                uniform_wds.dstArrayElement = 0;

                uniform_wds.descriptorType = vk::DescriptorType::eUniformBuffer;
                uniform_wds.descriptorCount = 1;
                uniform_wds.pBufferInfo = &(infos[i][c].dbi);
                uniform_wds.pImageInfo = nullptr;
                uniform_wds.pTexelBufferView = nullptr;
                wds_s.push_back(uniform_wds);

                for (uint32_t j = set_copies; j < infos[i].size(); ++j)
                {
                    vk::WriteDescriptorSet wds{};
                    wds.sType = vk::StructureType::eWriteDescriptorSet;
                    wds.dstSet = sets[(i * set_copies) + c];
                    wds.dstBinding = layout_bindings[i][j - set_copies + 1].binding;
                    wds.dstArrayElement = 0;

                    wds.descriptorType = layout_bindings[i][j - set_copies + 1].descriptorType;
                    wds.descriptorCount = 1;
                    wds.pBufferInfo = &(infos[i][j].dbi);
                    wds.pImageInfo = &(infos[i][j].dii);
                    wds.pTexelBufferView = nullptr;

                    wds_s.push_back(wds);
                }
            }
        }
        vmc.logical_device.get().updateDescriptorSets(wds_s, {});
    }

    void DescriptorSetHandler::self_destruct()
    {
        vmc.logical_device.get().destroyDescriptorPool(pool);
        for (auto& dsl: layouts)
        {
            vmc.logical_device.get().destroyDescriptorSetLayout(dsl);
        }
    }

    const std::vector<vk::DescriptorSetLayout>& DescriptorSetHandler::get_layouts() const
    {
        return layouts;
    }

    const std::vector<vk::DescriptorSet>& DescriptorSetHandler::get_sets() const
    {
        return sets;
    }

    void DescriptorSetHandler::add_layout_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages)
    {
        vk::DescriptorSetLayoutBinding dslb{};
        dslb.binding = binding;
        dslb.descriptorType = type;
        dslb.descriptorCount = 1;
        dslb.stageFlags = stages;
        dslb.pImmutableSamplers = nullptr;

        layout_bindings.back().push_back(dslb);
    }
}// namespace ve
