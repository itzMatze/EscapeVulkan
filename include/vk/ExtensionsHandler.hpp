#pragma once

#include <cstdint>
#include <vector>

namespace ve
{
    class ExtensionsHandler
    {
    public:
        ExtensionsHandler() = default;
        const std::vector<const char*>& get_missing_extensions() const;
        const std::vector<const char*>& get_extensions() const;
        uint32_t get_size() const;
        void add_extensions(const std::vector<const char*>& requested_extensions, bool required);
        int32_t check_extension_availability(const std::vector<const char*>& available_extensions);
        void remove_missing_extensions();
        bool find_extension(const char* name) const;

    private:
        std::vector<bool> ext_required;
        std::vector<const char*> extensions;
        std::vector<const char*> missing_extensions;

        bool find_extension(const char* name, const std::vector<const char*>& extensions) const;
    };
} // namespace ve