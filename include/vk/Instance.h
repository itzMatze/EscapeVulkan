#pragma once

#include "vulkan/vulkan.hpp"
#include <unordered_set>

#include "Window.h"
#include "ve_log.h"
#include "vk/ExtensionsHandler.h"

namespace ve
{
    class Instance
    {
    public:
        Instance(const Window& window, const std::vector<const char*>& required_extensions, const std::vector<const char*>& optional_extensions, const std::vector<const char*>& validation_layers) : extensions_handler()
        {
            VE_LOG_CONSOLE("Creating instance");
            vk::ApplicationInfo ai{};
            ai.sType = vk::StructureType::eApplicationInfo;
            ai.pApplicationName = "Vulkan Engine";
            ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            ai.pEngineName = "No Engine";
            ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            ai.apiVersion = VK_API_VERSION_1_3;

            uint32_t available_extension_count = 0;
            VE_CHECK(vk::enumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr), "Failed to get available extensions count!");

            std::vector<vk::ExtensionProperties> available_extensions(available_extension_count);
            VE_CHECK(vk::enumerateInstanceExtensionProperties(nullptr, &available_extension_count, available_extensions.data()), "Failed to get available extensions!");

            std::vector<const char*> tmp_extensions;
            window.get_required_extensions(tmp_extensions);
            extensions_handler.add_extensions(available_extensions, tmp_extensions, true);
            extensions_handler.add_extensions(available_extensions, required_extensions, true);
            extensions_handler.add_extensions(available_extensions, optional_extensions, false);

            std::vector<const char*> enabled_validation_layers;
            if (!get_available_validation_layers(validation_layers, enabled_validation_layers)) VE_WARN_CONSOLE("No validation layers added!");

            vk::InstanceCreateInfo ici{};
            ici.sType = vk::StructureType::eInstanceCreateInfo;
            ici.pApplicationInfo = &ai;
            ici.enabledExtensionCount = extensions_handler.get_extensions().size();
            ici.ppEnabledExtensionNames = extensions_handler.get_extensions().data();
            ici.enabledLayerCount = enabled_validation_layers.size();
            ici.ppEnabledLayerNames = enabled_validation_layers.data();

            VE_CHECK(vk::createInstance(&ici, nullptr, &instance), "Instance creation failed!");

            VE_ASSERT(SDL_Vulkan_CreateSurface(window.get(), instance, reinterpret_cast<VkSurfaceKHR*>(&surface)), "Failed to create surface!");
        }

        ~Instance()
        {
            instance.destroySurfaceKHR(surface);
            instance.destroy();
        }

        vk::Instance get() const
        {
            return instance;
        }

        vk::SurfaceKHR get_surface() const
        {
            return surface;
        }

        const std::vector<const char*>& get_missing_extensions() const
        {
            return extensions_handler.get_missing_extensions();
        }

        std::vector<vk::PhysicalDevice> get_physical_devices() const
        {
            uint32_t device_count = 0;
            VE_CHECK(instance.enumeratePhysicalDevices(&device_count, nullptr), "Getting physical device count failed!");
            if (device_count == 0) VE_THROW("Failed to find GPUs with Vulkan support!");
            std::vector<vk::PhysicalDevice> physical_devices(device_count);
            VE_CHECK(instance.enumeratePhysicalDevices(&device_count, physical_devices.data()), "Physical device enumeration failed!");
            return physical_devices;
        }

    private:
        bool get_available_validation_layers(const std::vector<const char*>& requested_validation_layers, std::vector<const char*>& validation_layers) const
        {
            uint32_t layer_count;
            VE_CHECK(vk::enumerateInstanceLayerProperties(&layer_count, nullptr), "Could not get available layers count!");

            std::vector<vk::LayerProperties> available_layers(layer_count);
            VE_CHECK(vk::enumerateInstanceLayerProperties(&layer_count, available_layers.data()), "Could not get available layers!");

            for (const auto& req_layer: requested_validation_layers)
            {
                for (const auto& avail_layer: available_layers)
                {
                    if (strcmp(req_layer, avail_layer.layerName) == 0)
                    {
                        validation_layers.push_back(req_layer);
                        break;
                    }
                }
            }
            return validation_layers.size() > 0;
        }

        vk::Instance instance;
        ExtensionsHandler extensions_handler;
        vk::SurfaceKHR surface;
    };
}// namespace ve