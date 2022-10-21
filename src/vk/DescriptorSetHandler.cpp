#include "vk/DescriptorSetHandler.hpp"

namespace ve
{
    DescriptorSetHandler::DescriptorSetHandler(const VulkanMainContext& vmc) : vmc(vmc)
    {}

    uint32_t DescriptorSetHandler::new_set()
    {
        layouts_bindings.push_back({});
        layouts_bindings.back().insert(layouts_bindings.back().end(), new_set_layout_bindings.begin(), new_set_layout_bindings.end());
        bindings.push_back({});
        bindings.back().insert(bindings.back().end(), new_set_bindings.begin(), new_set_bindings.end());
        return (bindings.size() - 1);
    }

    void DescriptorSetHandler::add_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, const vk::Buffer& buffer, uint64_t byte_size)
    {
        vk::DescriptorSetLayoutBinding dslb{};
        dslb.binding = binding;
        dslb.descriptorType = type;
        dslb.descriptorCount = 1;
        dslb.stageFlags = stages;
        dslb.pImmutableSamplers = nullptr;

        layouts_bindings.back().push_back(dslb);

        vk::DescriptorBufferInfo dbi{};
        dbi.buffer = buffer;
        dbi.offset = 0;
        dbi.range = byte_size;

        bindings.back().push_back(Binding(dbi, {}));
    }

    void DescriptorSetHandler::add_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, const Image& image)
    {
        vk::DescriptorSetLayoutBinding dslb{};
        dslb.binding = binding;
        dslb.descriptorType = type;
        dslb.descriptorCount = 1;
        dslb.stageFlags = stages;
        dslb.pImmutableSamplers = nullptr;

        layouts_bindings.back().push_back(dslb);

        vk::DescriptorImageInfo dii{};
        dii.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        dii.imageView = image.get_view();
        dii.sampler = image.get_sampler();

        bindings.back().push_back(Binding({}, dii));
    }

    void DescriptorSetHandler::apply_binding_to_new_sets(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, const vk::Buffer& buffer, uint64_t byte_size)
    {
        vk::DescriptorSetLayoutBinding dslb{};
        dslb.binding = binding;
        dslb.descriptorType = type;
        dslb.descriptorCount = 1;
        dslb.stageFlags = stages;
        dslb.pImmutableSamplers = nullptr;

        new_set_layout_bindings.push_back(dslb);

        vk::DescriptorBufferInfo dbi{};
        dbi.buffer = buffer;
        dbi.offset = 0;
        dbi.range = byte_size;

        new_set_bindings.push_back(Binding(dbi, {}));
    }

    void DescriptorSetHandler::reset_auto_apply_bindings()
    {
        new_set_layout_bindings.clear();
        new_set_bindings.clear();
    }

    void DescriptorSetHandler::construct()
    {
        std::vector<vk::DescriptorPoolSize> pool_sizes;
        for (const auto& layout_bindings: layouts_bindings)
        {
            for (const auto& dslb: layout_bindings)
            {
                vk::DescriptorPoolSize dps{};
                dps.type = dslb.descriptorType;
                dps.descriptorCount = dslb.descriptorCount;

                pool_sizes.push_back(dps);
            }

            vk::DescriptorSetLayoutCreateInfo dslci{};
            dslci.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
            dslci.bindingCount = layout_bindings.size();
            dslci.pBindings = layout_bindings.data();
            layouts.push_back(vmc.logical_device.get().createDescriptorSetLayout(dslci));
        }

        vk::DescriptorPoolCreateInfo dpci{};
        dpci.sType = vk::StructureType::eDescriptorPoolCreateInfo;
        dpci.poolSizeCount = pool_sizes.size();
        dpci.pPoolSizes = pool_sizes.data();
        dpci.maxSets = bindings.size();

        pool = vmc.logical_device.get().createDescriptorPool(dpci);

        vk::DescriptorSetAllocateInfo dsai{};
        dsai.sType = vk::StructureType::eDescriptorSetAllocateInfo;
        dsai.descriptorPool = pool;
        dsai.descriptorSetCount = layouts.size();
        dsai.pSetLayouts = layouts.data();

        sets = vmc.logical_device.get().allocateDescriptorSets(dsai);

        std::vector<vk::WriteDescriptorSet> wds_s;
        for (uint32_t i = 0; i < bindings.size(); ++i)
        {
            for (uint32_t j = 0; j < bindings[i].size(); ++j)
            {
                vk::WriteDescriptorSet wds{};
                wds.sType = vk::StructureType::eWriteDescriptorSet;
                wds.dstSet = sets[i];
                wds.dstBinding = layouts_bindings[i][j].binding;
                wds.dstArrayElement = 0;

                wds.descriptorType = layouts_bindings[i][j].descriptorType;
                wds.descriptorCount = 1;
                wds.pBufferInfo = &(bindings[i][j].dbi);
                wds.pImageInfo = &(bindings[i][j].dii);
                wds.pTexelBufferView = nullptr;

                wds_s.push_back(wds);
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

}// namespace ve
