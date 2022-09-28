#pragma once

#include "vulkan/vulkan.hpp"
#include <unordered_set>

#include "Window.h"

namespace ve
{
    class Instance
    {
    public:
        Instance(Window* window, const std::vector<const char*>& required_extensions, const std::vector<const char*>& optional_extensions)
        {
            vk::ApplicationInfo ai{};
            ai.sType = vk::StructureType::eApplicationInfo;
            ai.pApplicationName = "Vulkan Engine";
            ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            ai.pEngineName = "No Engine";
            ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            ai.apiVersion = VK_API_VERSION_1_3;

            uint32_t extension_count = 0;
            uint32_t tmp_extension_count = 0;
            std::vector<const char*> extensions;
            std::vector<const char*> tmp_extensions;
            if (!window->get_required_extensions(tmp_extension_count, tmp_extensions)) throw std::runtime_error("Could not load required extensions for window!");
            extensions.insert(extensions.end(), tmp_extensions.begin(), tmp_extensions.end());
            extension_count += tmp_extension_count;
            add_extensions(tmp_extension_count, tmp_extensions, required_extensions, optional_extensions);

            vk::InstanceCreateInfo ici{};
            ici.sType = vk::StructureType::eInstanceCreateInfo;
            ici.pApplicationInfo = &ai;
            ici.enabledExtensionCount = extension_count;
            ici.ppEnabledExtensionNames = extensions.data();
            ici.enabledLayerCount = 0;

            vk::resultCheck(vk::createInstance(&ici, nullptr, &instance), "Instance creation failed!");
        }

        ~Instance()
        {
        	instance.destroy();
        }

        std::vector<const char*> get_missing_extensions()
        {
            return missing_extensions;
        }

    private:
        std::vector<const char*> add_extensions(uint32_t& extension_count, std::vector<const char*>& extensions, const std::vector<const char*>& required_extensions, const std::vector<const char*>& optional_extensions)
        {
            uint32_t available_extension_count = 0;
            vk::resultCheck(vk::enumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr), "Could not get available extensions count!");

            std::vector<vk::ExtensionProperties> available_extensions(available_extension_count);
            vk::resultCheck(vk::enumerateInstanceExtensionProperties(nullptr, &available_extension_count, available_extensions.data()), "Could not get available extensions!");

            std::unordered_set<const char*> available_extensions_set;
            for (auto& extension: available_extensions)
            {
                available_extensions_set.insert(extension.extensionName);
            }

            for (auto& extension: required_extensions)
            {
                if (!available_extensions_set.contains(extension)) throw std::runtime_error("Required extension unavailable!");
            }

            for (auto& extension: optional_extensions)
            {
                if (!available_extensions_set.contains(extension))
                {
                    std::cout << "Optional extension " << extension << " unavailable!" << std::endl;
                    missing_extensions.push_back(extension);
                }
            }
            return missing_extensions;
        }

        vk::Instance instance;
        std::vector<const char*> missing_extensions;
    };
}// namespace ve