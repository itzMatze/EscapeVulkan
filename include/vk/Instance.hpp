#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "vk/ExtensionsHandler.hpp"

namespace ve
{
    class Instance
    {
    public:
        Instance(std::vector<const char*> required_extensions);
        void self_destruct();
        const vk::Instance& get() const;
        const std::vector<const char*>& get_missing_extensions() const;
        std::vector<vk::PhysicalDevice> get_physical_devices() const;

    private:
        vk::Instance instance;
        ExtensionsHandler extensions_handler;
        ExtensionsHandler validation_handler;
    };
} // namespace ve
