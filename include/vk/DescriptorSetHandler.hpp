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
        void add_uniform_buffer(uint32_t copies, const std::vector<Buffer>& data);
        void add_buffer_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, const vk::Buffer& buffer, uint64_t byte_size);
        void add_image_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, const Image& image);
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
        // copies defines how many frames there are in flight. Every frame needs its own uniform buffer, so they do not interfere
        uint32_t set_copies = 0;
        // the first set_copies elements are the uniform buffers
        std::vector<Binding> infos;
        std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
        std::vector<vk::DescriptorSetLayout> layouts;
        vk::DescriptorPool pool;
        std::vector<vk::DescriptorSet> sets;

        void add_layout_binding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages);
    };
}// namespace ve
