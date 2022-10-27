#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/Buffer.hpp"
#include "vk/Image.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
    class DescriptorSetHandler
    {
    public:
        DescriptorSetHandler(const VulkanMainContext& vmc);
        uint32_t new_set();
        void add_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages);
        void add_descriptor(uint32_t binding, const Image& image);
        void add_descriptor(uint32_t binding, const Buffer& buffer);
        void apply_descriptor_to_new_sets(uint32_t binding, const Buffer& buffer);
        void reset_auto_apply_bindings();
        void construct();
        void self_destruct();
        const std::vector<vk::DescriptorSetLayout>& get_layouts() const;
        const std::vector<vk::DescriptorSet>& get_sets() const;

    private:
        struct Descriptor {
            Descriptor(uint32_t binding, vk::DescriptorBufferInfo dbi, vk::DescriptorImageInfo dii) : binding(binding), dbi(dbi), dii(dii)
            {}
            uint32_t binding;
            vk::DescriptorBufferInfo dbi;
            vk::DescriptorImageInfo dii;
            bool operator<(const Descriptor& b) const
            {
                return (binding < b.binding);
            }
        };

        const VulkanMainContext& vmc;
        std::vector<Descriptor> new_set_descriptors;
        std::vector<std::vector<Descriptor>> descriptor_sets;
        std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
        std::vector<vk::DescriptorSetLayout> layouts;
        vk::DescriptorPool pool;
        std::vector<vk::DescriptorSet> sets;
    };
}// namespace ve
