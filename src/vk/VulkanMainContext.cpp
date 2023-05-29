#include "vk/VulkanMainContext.hpp"

#include "ve_log.hpp"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{

    std::cerr << "validation layer: " << callback_data->pMessage << std::endl;

    return VK_FALSE;
}

namespace ve
{
    // create VulkanMainContext without window for non graphical applications
    VulkanMainContext::VulkanMainContext() : instance({}), physical_device(instance, surface), queue_family_indices(physical_device.get_queue_families(surface)), logical_device(physical_device, queue_family_indices, queues)
    {
        create_vma_allocator();
        setup_debug_messenger();
        spdlog::info("Created VulkanMainContext");
    }

    // create VulkanMainContext with window for graphical applications
    VulkanMainContext::VulkanMainContext(const uint32_t width, const uint32_t height) : window(std::make_optional<Window>(width, height)), instance(window->get_required_extensions()), surface(window->create_surface(instance.get())), physical_device(instance, surface), queue_family_indices(physical_device.get_queue_families(surface)), logical_device(physical_device, queue_family_indices, queues)
    {
        create_vma_allocator();
        spdlog::info("Created VulkanMainContext");
    }

    void VulkanMainContext::self_destruct()
    {
        vmaDestroyAllocator(va);
        if (surface.has_value()) instance.get().destroySurfaceKHR(surface.value());
        logical_device.self_destruct();
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) instance.get().getProcAddr("vkDestroyDebugUtilsMessengerEXT");
        func(instance.get(), debug_messenger, nullptr);
        instance.self_destruct();
        if (window.has_value()) window->self_destruct();
        spdlog::info("Destroyed VulkanMainContext");
    }

    std::vector<vk::SurfaceFormatKHR> VulkanMainContext::get_surface_formats() const
    {
        if (!surface.has_value()) VE_THROW("Surface not initialized!");
        return physical_device.get().getSurfaceFormatsKHR(surface.value());
    }

    std::vector<vk::PresentModeKHR> VulkanMainContext::get_surface_present_modes() const
    {
        if (!surface.has_value()) VE_THROW("Surface not initialized!");
        return physical_device.get().getSurfacePresentModesKHR(surface.value());
    }

    vk::SurfaceCapabilitiesKHR VulkanMainContext::get_surface_capabilities() const
    {
        if (!surface.has_value()) VE_THROW("Surface not initialized!");
        return physical_device.get().getSurfaceCapabilitiesKHR(surface.value());
    }

    const vk::Queue& VulkanMainContext::get_graphics_queue() const
    {
        return queues.at(QueueIndex::Graphics);
    }

    const vk::Queue& VulkanMainContext::get_transfer_queue() const
    {
        return queues.at(QueueIndex::Transfer);
    }

    const vk::Queue& VulkanMainContext::get_compute_queue() const
    {
        return queues.at(QueueIndex::Compute);
    }

    const vk::Queue& VulkanMainContext::get_present_queue() const
    {
        return queues.at(QueueIndex::Present);
    }

    void VulkanMainContext::create_vma_allocator()
    {
        VmaAllocatorCreateInfo vaci{};
        vaci.instance = instance.get();
        vaci.physicalDevice = physical_device.get();
        vaci.device = logical_device.get();
        vaci.vulkanApiVersion = VK_API_VERSION_1_3;
        vmaCreateAllocator(&vaci, &va);
    }

    void VulkanMainContext::setup_debug_messenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT dumci;
        dumci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dumci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dumci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dumci.pfnUserCallback = debug_callback;
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) instance.get().getProcAddr("vkCreateDebugUtilsMessengerEXT");
        func(instance.get(), &dumci, nullptr, (VkDebugUtilsMessengerEXT*)(&debug_messenger));
    }
} // namespace ve
