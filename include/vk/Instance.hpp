#pragma once

#include "vulkan/vulkan.hpp"
#include <unordered_set>

#include "Window.hpp"
#include "ve_log.hpp"
#include "vk/ExtensionsHandler.hpp"

namespace ve
{
    class Instance
    {
    public:
        Instance(const Window& window, const std::vector<const char*>& required_extensions, const std::vector<const char*>& optional_extensions, const std::vector<const char*>& validation_layers) : extensions_handler()
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "instance +\n");
            vk::ApplicationInfo ai{};
            ai.sType = vk::StructureType::eApplicationInfo;
            ai.pApplicationName = "Vulkan Engine";
            ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            ai.pEngineName = "No Engine";
            ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            ai.apiVersion = VK_API_VERSION_1_3;

            std::vector<vk::ExtensionProperties> available_extensions = vk::enumerateInstanceExtensionProperties();

            std::vector<const char*> tmp_extensions;
            window.get_required_extensions(tmp_extensions);
            extensions_handler.add_extensions(available_extensions, tmp_extensions, true);
            extensions_handler.add_extensions(available_extensions, required_extensions, true);
            extensions_handler.add_extensions(available_extensions, optional_extensions, false);

            std::vector<const char*> enabled_validation_layers;
            if (!get_available_validation_layers(validation_layers, enabled_validation_layers)) VE_LOG_CONSOLE(VE_WARN, VE_C_YELLOW << "No validation layers added!\n");

            vk::InstanceCreateInfo ici{};
            ici.sType = vk::StructureType::eInstanceCreateInfo;
            ici.pApplicationInfo = &ai;
            ici.enabledExtensionCount = extensions_handler.get_extensions().size();
            ici.ppEnabledExtensionNames = extensions_handler.get_extensions().data();
            ici.enabledLayerCount = enabled_validation_layers.size();
            ici.ppEnabledLayerNames = enabled_validation_layers.data();

            VE_LOG_CONSOLE(VE_INFO, "Creating instance\n");
            instance = vk::createInstance(ici);

            VE_LOG_CONSOLE(VE_INFO, "Creating surface\n");
            VE_ASSERT(SDL_Vulkan_CreateSurface(window.get(), instance, reinterpret_cast<VkSurfaceKHR*>(&surface)), "Failed to create surface!");
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "instance +++\n");
        }

        ~Instance()
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "instance -\n");
            VE_LOG_CONSOLE(VE_INFO, "Destroying surface\n");
            instance.destroySurfaceKHR(surface);
            VE_LOG_CONSOLE(VE_INFO, "Destroying instance\n");
            instance.destroy();
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "instance ---\n");
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
            std::vector<vk::PhysicalDevice>physical_devices = instance.enumeratePhysicalDevices();
            if (physical_devices.empty()) VE_THROW("Failed to find GPUs with Vulkan support!");
            return physical_devices;
        }

    private:
        bool get_available_validation_layers(const std::vector<const char*>& requested_validation_layers, std::vector<const char*>& validation_layers) const
        {
            std::vector<vk::LayerProperties> available_layers = vk::enumerateInstanceLayerProperties();

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