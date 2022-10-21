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
        void add_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, const vk::Buffer& buffer, uint64_t byte_size);
        void add_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, const Image& image);
        void apply_binding_to_new_sets(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, const vk::Buffer& buffer, uint64_t byte_size);
        void reset_auto_apply_bindings();
        void construct();
        void self_destruct();
        const std::vector<vk::DescriptorSetLayout>& get_layouts() const;
        const std::vector<vk::DescriptorSet>& get_sets() const;

    private:
        struct Binding {
            Binding(vk::DescriptorBufferInfo dbi, vk::DescriptorImageInfo dii) : dbi(dbi), dii(dii)
            {}
            vk::DescriptorBufferInfo dbi;
            vk::DescriptorImageInfo dii;
        };

        const VulkanMainContext& vmc;
        std::vector<Binding> new_set_bindings;
        std::vector<std::vector<Binding>> bindings;
        std::vector<vk::DescriptorSetLayoutBinding> new_set_layout_bindings;
        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> layouts_bindings;
        std::vector<vk::DescriptorSetLayout> layouts;
        vk::DescriptorPool pool;
        std::vector<vk::DescriptorSet> sets;
    };
}// namespace ve
