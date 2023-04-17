#include "vk/DescriptorSetHandler.hpp"

namespace ve
{
    DescriptorSetHandler::DescriptorSetHandler(const VulkanMainContext& vmc) : vmc(vmc)
    {}

    uint32_t DescriptorSetHandler::new_set()
    {
        // add new descriptor set and apply current auto apply bindings
        descriptor_sets.push_back({});
        descriptor_sets.back().insert(descriptor_sets.back().end(), new_set_descriptors.begin(), new_set_descriptors.end());
        return (descriptor_sets.size() - 1);
    }

    void DescriptorSetHandler::add_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages)
    {
        // add binding to build descriptor set layout
        vk::DescriptorSetLayoutBinding dslb{};
        dslb.binding = binding;
        dslb.descriptorType = type;
        dslb.descriptorCount = 1;
        dslb.stageFlags = stages;
        dslb.pImmutableSamplers = nullptr;
        layout_bindings.push_back(dslb);
    }

    void DescriptorSetHandler::add_descriptor(uint32_t binding, const Buffer& buffer)
    {
        // add buffer descriptor to current descriptor set
        // every set needs to be build one after another
        vk::DescriptorBufferInfo dbi{};
        dbi.buffer = buffer.get();
        dbi.offset = 0;
        dbi.range = buffer.get_byte_size();
        descriptor_sets.back().push_back(Descriptor(binding, dbi, {}));
    }

    void DescriptorSetHandler::add_descriptor(uint32_t binding, const Image& image)
    {
        // add image descriptor to current descriptor set
        // every set needs to be build one after another
        vk::DescriptorImageInfo dii{};
        dii.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        dii.imageView = image.get_view();
        dii.sampler = image.get_sampler();
        descriptor_sets.back().push_back(Descriptor(binding, {}, dii));
    }

    void DescriptorSetHandler::apply_descriptor_to_new_sets(uint32_t binding, const Buffer& buffer)
    {
        // add buffer descriptor to be added to every new descriptor set (used for e.g. uniform buffers)
        vk::DescriptorBufferInfo dbi{};
        dbi.buffer = buffer.get();
        dbi.offset = 0;
        dbi.range = buffer.get_byte_size();
        new_set_descriptors.push_back(Descriptor(binding, dbi, {}));
    }

    void DescriptorSetHandler::reset_auto_apply_bindings()
    {
        new_set_descriptors.clear();
    }

    void DescriptorSetHandler::construct()
    {
        // sort with respect to binding that the layout and the descriptor sets are in the same order
        std::sort(layout_bindings.begin(), layout_bindings.end());
        for (auto& descriptors : descriptor_sets)
        {
            std::sort(descriptors.begin(), descriptors.end());
        }

        for (uint32_t i = 0; i < descriptor_sets.size(); ++i)
        {
            vk::DescriptorSetLayoutCreateInfo dslci{};
            dslci.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
            dslci.bindingCount = layout_bindings.size();
            dslci.pBindings = layout_bindings.data();
            layouts.push_back(vmc.logical_device.get().createDescriptorSetLayout(dslci));
        }

        std::vector<vk::DescriptorPoolSize> pool_sizes;
        for (const auto& dslb : layout_bindings)
        {
            vk::DescriptorPoolSize dps{};
            dps.type = dslb.descriptorType;
            dps.descriptorCount = descriptor_sets.size();
            pool_sizes.push_back(dps);
        }

        vk::DescriptorPoolCreateInfo dpci{};
        dpci.sType = vk::StructureType::eDescriptorPoolCreateInfo;
        dpci.poolSizeCount = pool_sizes.size();
        dpci.pPoolSizes = pool_sizes.data();
        dpci.maxSets = descriptor_sets.size();

        pool = vmc.logical_device.get().createDescriptorPool(dpci);

        vk::DescriptorSetAllocateInfo dsai{};
        dsai.sType = vk::StructureType::eDescriptorSetAllocateInfo;
        dsai.descriptorPool = pool;
        dsai.descriptorSetCount = layouts.size();
        dsai.pSetLayouts = layouts.data();

        sets = vmc.logical_device.get().allocateDescriptorSets(dsai);

        std::vector<vk::WriteDescriptorSet> wds_s;
        for (uint32_t i = 0; i < descriptor_sets.size(); ++i)
        {
            for (uint32_t j = 0; j < descriptor_sets[i].size(); ++j)
            {
                vk::WriteDescriptorSet wds{};
                wds.sType = vk::StructureType::eWriteDescriptorSet;
                wds.dstSet = sets[i];
                wds.dstBinding = layout_bindings[j].binding;
                wds.dstArrayElement = 0;

                // descriptorType decides if descriptor is buffer or image, the unused one is empty
                wds.descriptorType = layout_bindings[j].descriptorType;
                wds.descriptorCount = 1;
                wds.pBufferInfo = &(descriptor_sets[i][j].dbi);
                wds.pImageInfo = &(descriptor_sets[i][j].dii);
                wds.pTexelBufferView = nullptr;

                wds_s.push_back(wds);
            }
        }
        vmc.logical_device.get().updateDescriptorSets(wds_s, {});
    }

    void DescriptorSetHandler::self_destruct()
    {
        for (auto& dsl : layouts)
        {
            vmc.logical_device.get().destroyDescriptorSetLayout(dsl);
        }
        layouts.clear();
        vmc.logical_device.get().destroyDescriptorPool(pool);
    }

    const std::vector<vk::DescriptorSetLayout>& DescriptorSetHandler::get_layouts() const
    {
        return layouts;
    }

    const std::vector<vk::DescriptorSet>& DescriptorSetHandler::get_sets() const
    {
        return sets;
    }

} // namespace ve
