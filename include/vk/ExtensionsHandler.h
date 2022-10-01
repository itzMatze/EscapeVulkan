#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.h"

class ExtensionsHandler
{
public:
    ExtensionsHandler() = default;

    const std::vector<const char*>& get_missing_extensions() const
    {
        return missing_extensions;
    }

    const std::vector<const char*>& get_extensions() const
    {
        return extensions;
    }

    void add_extensions(const std::vector<vk::ExtensionProperties>& available_extensions, const std::vector<const char*>& requested_extensions, bool required)
    {
        for (const auto& requested_extension: requested_extensions)
        {
            if (is_extension_available(requested_extension, available_extensions))
            {
                extensions.push_back(requested_extension);
            }
            else
            {
                if (required)
                {
                    VE_THROW("Required extension \"" << requested_extension << "\" unavailable!");
                }
                else
                {
                    VE_WARN_CONSOLE("Optional extension \"" << requested_extension << "\" unavailable!");
                    missing_extensions.push_back(requested_extension);
                }
            }
        }
    }

    bool is_extension_available(const char* requested_extension, const std::vector<vk::ExtensionProperties>& available_extensions) const
    {
        bool found = false;
        for (const auto& avail_extension: available_extensions)
        {
            if (strcmp(requested_extension, avail_extension.extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        return found;
    }

private:
    std::vector<const char*> extensions;
    std::vector<const char*> missing_extensions;
};