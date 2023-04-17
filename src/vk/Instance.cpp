#include "vk/Instance.hpp"

#include "Window.hpp"
#include "ve_log.hpp"

namespace ve
{
    Instance::Instance(std::vector<const char*> required_extensions) : extensions_handler()
    {
        required_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        const std::vector<const char*> optional_extensions{};
        const std::vector<const char*> validation_layers{"VK_LAYER_KHRONOS_validation"};

        vk::ApplicationInfo ai{};
        ai.sType = vk::StructureType::eApplicationInfo;
        ai.pApplicationName = "Vulkan Engine";
        ai.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        ai.pEngineName = "No Engine";
        ai.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        ai.apiVersion = VK_API_VERSION_1_3;

        // use ExtensionHandler class to check if extensions and validation layers are available
        std::vector<vk::ExtensionProperties> available_extensions = vk::enumerateInstanceExtensionProperties();
        std::vector<const char*> avail_ext_names;
        for (const auto& ext : available_extensions) avail_ext_names.push_back(ext.extensionName);
        extensions_handler.add_extensions(required_extensions, true);
        extensions_handler.add_extensions(optional_extensions, false);
        if (extensions_handler.check_extension_availability(avail_ext_names) == -1) VE_THROW("Required instance extension not found!");
        extensions_handler.remove_missing_extensions();

        std::vector<vk::LayerProperties> available_layers = vk::enumerateInstanceLayerProperties();
        std::vector<const char*> avail_layer_names;
        for (const auto& layer : available_layers) avail_layer_names.push_back(layer.layerName);
        validation_handler.add_extensions(validation_layers, false);
        int32_t missing_layers = validation_handler.check_extension_availability(avail_layer_names);
        validation_handler.remove_missing_extensions();
        if (missing_layers > 0) spdlog::warn("{} validation layers not found!", missing_layers);

        vk::InstanceCreateInfo ici{};
        ici.sType = vk::StructureType::eInstanceCreateInfo;
        ici.pApplicationInfo = &ai;
        ici.enabledExtensionCount = extensions_handler.get_size();
        ici.ppEnabledExtensionNames = extensions_handler.get_extensions().data();
        ici.enabledLayerCount = validation_handler.get_size();
        ici.ppEnabledLayerNames = validation_handler.get_extensions().data();

        instance = vk::createInstance(ici);
    }

    void Instance::self_destruct()
    {
        instance.destroy();
    }

    const vk::Instance& Instance::get() const
    {
        return instance;
    }

    const std::vector<const char*>& Instance::get_missing_extensions() const
    {
        return extensions_handler.get_missing_extensions();
    }

    std::vector<vk::PhysicalDevice> Instance::get_physical_devices() const
    {
        std::vector<vk::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();
        if (physical_devices.empty()) VE_THROW("Failed to find GPUs with Vulkan support!");
        return physical_devices;
    }
} // namespace ve
