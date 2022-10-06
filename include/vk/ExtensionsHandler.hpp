#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"

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

    uint32_t get_size() const
    {
        return extensions.size();
    }

    void add_extensions(const std::vector<const char*>& requested_extensions, bool required)
    {
        extensions.insert(extensions.end(), requested_extensions.begin(), requested_extensions.end());
        ext_required.resize(ext_required.size() + requested_extensions.size(), required);
    }

    int32_t check_extension_availability(const std::vector<const char*>& available_extensions)
    {
        missing_extensions.clear();
        for (uint32_t i = 0; i < extensions.size(); ++i)
        {
            if (!find_extension(extensions[i], available_extensions))
            {
                if (ext_required[i])
                {
                    return -1;
                }
                else
                {
                    missing_extensions.push_back(extensions[i]);
                }
            }
        }
        return missing_extensions.size();
    }

    void remove_missing_extensions()
    {
        std::vector<const char*> tmp_exts(extensions);
        extensions.clear();
        for (const auto& ext: tmp_exts)
        {
            if (!find_extension(ext, missing_extensions)) extensions.push_back(ext);
        }
    }

    bool find_extension(const char* name) const
    {
        return find_extension(name, extensions);
    }

private:
    bool find_extension(const char* name, const std::vector<const char*>& extensions) const
    {
        for (const auto& ext: extensions)
        {
            if (strcmp(ext, name) == 0)
            {
                return true;
            }
        }
        return false;
    }

    std::vector<bool> ext_required;
    std::vector<const char*> extensions;
    std::vector<const char*> missing_extensions;
};