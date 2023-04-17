#pragma once

#include <vector>
#include <unordered_map>
#include "vk/Buffer.hpp"
#include "vk/Image.hpp"

namespace ve
{
    class VulkanStorageContext
    {
    public:
        VulkanStorageContext(const VulkanMainContext& vmc, const VulkanCommandContext& vcc);

        template<typename... Args>
        uint32_t add_named_buffer(const std::string& name, Args&&... args)
        {
            buffers.emplace_back(std::make_optional<Buffer>(std::forward<Args>(args)...));
            if (buffer_names.contains(name))
            {
                if (buffers.at(buffer_names.at(name)).has_value())
                {
                    // buffer name is already taken by an existing buffer
                    spdlog::warn("Duplicate buffer name!");
                }
                else
                {
                    // buffer name exists but the corresponding buffer got deleted; so, reuse the name
                    buffer_names.at(name) = buffers.size() - 1;
                }
            }
            else
            {
                buffer_names.emplace(name, buffers.size() - 1);
            }
            return buffers.size() - 1;
        }

        template<typename... Args>
        uint32_t add_named_image(const std::string& name, Args&&... args)
        {
            images.emplace_back(std::make_optional<Image>(std::forward<Args>(args)...));
            if (buffer_names.contains(name))
            {
                if (images.at(image_names.at(name)).has_value())
                {
                    // image name is already taken by an existing image
                    spdlog::warn("Duplicate image name!");
                }
                else
                {
                    // image name exists but the corresponding image got deleted; so, reuse the name
                    image_names.at(name) = images.size() - 1;
                }
            }
            else
            {
                image_names.emplace(name, images.size() - 1);
            }
            return images.size() - 1;
        }

        template<typename... Args>
        uint32_t add_buffer(Args&&... args)
        {
            buffers.emplace_back(std::make_optional<Buffer>(std::forward<Args>(args)...));
            return buffers.size() - 1;
        }

        template<typename... Args>
        uint32_t add_image(Args&&... args)
        {
            images.emplace_back(std::make_optional<Image>(std::forward<Args>(args)...));
            return images.size() - 1;
        }

        void destroy_buffer(uint32_t idx);
        void destroy_image(uint32_t idx);
        void destroy_buffer(const std::string& name);
        void destroy_image(const std::string& name);
        void clear();
        Buffer& get_buffer(uint32_t idx);
        Image& get_image(uint32_t idx);
        Buffer& get_buffer_by_name(const std::string& name);
        Image& get_image_by_name(const std::string& name);

    private:
        const VulkanMainContext& vmc;
        const VulkanCommandContext& vcc;
        std::vector<std::optional<Buffer>> buffers;
        std::vector<std::optional<Image>> images;
        std::unordered_map<std::string, uint32_t> buffer_names;
        std::unordered_map<std::string, uint32_t> image_names;
    };
} // namespace ve
