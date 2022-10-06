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
        Instance(const Window& window) : extensions_handler()
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "instance +\n");
            const std::vector<const char*> required_extensions{VK_KHR_SURFACE_EXTENSION_NAME};
            const std::vector<const char*> optional_extensions{};
            const std::vector<const char*> validation_layers{"VK_LAYER_KHRONOS_validation"};

            vk::ApplicationInfo ai{};
            ai.sType = vk::StructureType::eApplicationInfo;
            ai.pApplicationName = "Vulkan Engine";
            ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            ai.pEngineName = "No Engine";
            ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            ai.apiVersion = VK_API_VERSION_1_3;

            std::vector<vk::ExtensionProperties> available_extensions = vk::enumerateInstanceExtensionProperties();
            std::vector<const char*> avail_ext_names;
            for (const auto& ext: available_extensions) avail_ext_names.push_back(ext.extensionName);
            std::vector<const char*> tmp_extensions;
            window.get_required_extensions(tmp_extensions);
            extensions_handler.add_extensions(tmp_extensions, true);
            extensions_handler.add_extensions(required_extensions, true);
            extensions_handler.add_extensions(optional_extensions, false);
            if (extensions_handler.check_extension_availability(avail_ext_names) == -1) VE_THROW("Required instance extension not found!");
            extensions_handler.remove_missing_extensions();

            std::vector<vk::LayerProperties> available_layers = vk::enumerateInstanceLayerProperties();
            std::vector<const char*> avail_layer_names;
            for (const auto& layer: available_layers) avail_layer_names.push_back(layer.layerName);
            validation_handler.add_extensions(validation_layers, false);
            int32_t missing_layers = validation_handler.check_extension_availability(avail_layer_names);
            validation_handler.remove_missing_extensions();
            if (missing_layers > 0) VE_LOG_CONSOLE(VE_WARN, VE_C_YELLOW << missing_layers << " validation layers not found!\n");

            vk::InstanceCreateInfo ici{};
            ici.sType = vk::StructureType::eInstanceCreateInfo;
            ici.pApplicationInfo = &ai;
            ici.enabledExtensionCount = extensions_handler.get_size();
            ici.ppEnabledExtensionNames = extensions_handler.get_extensions().data();
            ici.enabledLayerCount = validation_handler.get_size();
            ici.ppEnabledLayerNames = validation_handler.get_extensions().data();

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
            std::vector<vk::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();
            if (physical_devices.empty()) VE_THROW("Failed to find GPUs with Vulkan support!");
            return physical_devices;
        }

    private:
        vk::Instance instance;
        ExtensionsHandler extensions_handler;
        ExtensionsHandler validation_handler;
        vk::SurfaceKHR surface;
    };
}// namespace ve