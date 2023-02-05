#include "UI.hpp"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_sdl.h"

#include "ve_log.hpp"

namespace ve
{
    UI::UI(const VulkanMainContext& vmc, const RenderPass& render_pass, uint32_t frames) : vmc(vmc)
    {
        std::vector<vk::DescriptorPoolSize> pool_sizes =
        {
            { vk::DescriptorType::eSampler, 1000 },
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
            { vk::DescriptorType::eSampledImage, 1000 },
            { vk::DescriptorType::eStorageImage, 1000 },
            { vk::DescriptorType::eUniformTexelBuffer, 1000 },
            { vk::DescriptorType::eStorageTexelBuffer, 1000 },
            { vk::DescriptorType::eUniformBuffer, 1000 },
            { vk::DescriptorType::eStorageBuffer, 1000 },
            { vk::DescriptorType::eUniformBufferDynamic, 1000 },
            { vk::DescriptorType::eStorageBufferDynamic, 1000 },
            { vk::DescriptorType::eInputAttachment, 1000 }
        };

        vk::DescriptorPoolCreateInfo dpci{};
        dpci.sType = vk::StructureType::eDescriptorPoolCreateInfo;
        dpci.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        dpci.maxSets = 1000;
        dpci.poolSizeCount = pool_sizes.size();
        dpci.pPoolSizes = pool_sizes.data();

        imgui_pool = vmc.logical_device.get().createDescriptorPool(dpci);

        ImGui::CreateContext();

        //this initializes imgui for SDL
        ImGui_ImplSDL2_InitForVulkan(vmc.window.value().get());

        //this initializes imgui for Vulkan
        ImGui_ImplVulkan_InitInfo ii{};
        ii.Instance = vmc.instance.get();
        ii.PhysicalDevice = vmc.physical_device.get();
        ii.Device = vmc.logical_device.get();
        ii.Queue = vmc.get_graphics_queue();
        ii.DescriptorPool = imgui_pool;
        ii.MinImageCount = frames;
        ii.ImageCount = frames;
        if (render_pass.get_sample_count() == vk::SampleCountFlagBits::e1) ii.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        if (render_pass.get_sample_count() == vk::SampleCountFlagBits::e2) ii.MSAASamples = VK_SAMPLE_COUNT_2_BIT;
        if (render_pass.get_sample_count() == vk::SampleCountFlagBits::e4) ii.MSAASamples = VK_SAMPLE_COUNT_4_BIT;
        if (render_pass.get_sample_count() == vk::SampleCountFlagBits::e8) ii.MSAASamples = VK_SAMPLE_COUNT_8_BIT;
        if (render_pass.get_sample_count() == vk::SampleCountFlagBits::e16) ii.MSAASamples = VK_SAMPLE_COUNT_16_BIT;
        if (render_pass.get_sample_count() == vk::SampleCountFlagBits::e32) ii.MSAASamples = VK_SAMPLE_COUNT_32_BIT;
        if (render_pass.get_sample_count() == vk::SampleCountFlagBits::e64) ii.MSAASamples = VK_SAMPLE_COUNT_64_BIT;

        ImGui_ImplVulkan_Init(&ii, render_pass.get());
        ImGui::StyleColorsDark();
    }

    void UI::self_destruct()
    {
        vmc.logical_device.get().destroyDescriptorPool(imgui_pool);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL2_Shutdown();
    }

    void UI::upload_font_textures(const VulkanCommandContext& vcc)
    {
        vk::CommandBuffer cb = vcc.begin(vcc.graphics_cb[0]);
        ImGui_ImplVulkan_CreateFontsTexture(cb);
        vcc.submit_graphics(cb, true);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void UI::draw(vk::CommandBuffer& cb, DrawInfo& di)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame(vmc.window.value().get());
        ImGui::NewFrame();
        ImGui::Text((ve::to_string(di.time_diff * 1000, 4) + " ms; FPS: " + ve::to_string(1.0 / di.time_diff) + " (" + ve::to_string(di.frametime, 4) + " ms; FPS: " + ve::to_string(1000.0 / di.frametime) + ")").c_str());
        ImGui::Text("Navigation");
        ImGui::Separator();
        ImGui::Text("'W'A'S'D'Q'E': movement");
        ImGui::Text("'+'-': change movement speed");
        ImGui::Text("'+'-': change movement speed");
        ImGui::Text("'Mouse_L': move camera");
        ImGui::Text("'G': Show/Hide UI");
        ImGui::EndFrame();

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
    }
}// namespace ve
