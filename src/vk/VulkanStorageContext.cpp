#include "vk/VulkanStorageContext.hpp"

namespace ve
{
    VulkanStorageContext::VulkanStorageContext(const VulkanMainContext& vmc, const VulkanCommandContext& vcc) : vmc(vmc), vcc(vcc)
    {}

    void VulkanStorageContext::destroy_buffer(uint32_t idx)
    {

        if (buffers.at(idx).has_value())
        {
            buffers.at(idx).value().self_destruct();
            buffers.at(idx).reset();
        }
        else
        {
            spdlog::warn("Trying to destroy already destroyed buffer!");
        }
    }

    void VulkanStorageContext::destroy_image(uint32_t idx)
    {
        if (images.at(idx).has_value())
        {
            images.at(idx).value().self_destruct();
            images.at(idx).reset();
        }
        else
        {
            spdlog::warn("Trying to destroy already destroyed image!");
        }
    }

    void VulkanStorageContext::destroy_buffer(const std::string& name)
    {
        destroy_buffer(buffer_names.at(name));
    }

    void VulkanStorageContext::destroy_image(const std::string& name)
    {
        destroy_image(image_names.at(name));
    }

    void VulkanStorageContext::clear()
    {
        for (auto& b : buffers)
        {
            if (b.has_value()) b.value().self_destruct();
        }
        buffers.clear();
        for (auto& i : images)
        {
            if (i.has_value()) i.value().self_destruct();
        }
        images.clear();
        buffer_names.clear();
        image_names.clear();
    }

    Buffer& VulkanStorageContext::get_buffer(uint32_t idx)
    {
        if (buffers.at(idx).has_value())
        {
            return buffers.at(idx).value();
        }
        else
        {
            VE_THROW("Trying to get already destroyed buffer!");
        }
    }

    Image& VulkanStorageContext::get_image(uint32_t idx)
    {
        if (images.at(idx).has_value())
        {
            return images.at(idx).value();
        }
        else
        {
            VE_THROW("Trying to get already destroyed image!");
        }
    }

    Buffer& VulkanStorageContext::get_buffer_by_name(const std::string& name)
    {
        return get_buffer(buffer_names.at(name));
    }

    Image& VulkanStorageContext::get_image_by_name(const std::string& name)
    {
        return get_image(image_names.at(name));
    }
} // namespace ve
