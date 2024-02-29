#include "UI.hpp"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_sdl.h"
#include "implot.h"
#include "implot_internal.h"

#include "ve_log.hpp"
#include "vk/Timer.hpp"

namespace ve
{
    constexpr uint32_t plot_value_count = 1024;
    constexpr float update_weight = 0.1f;

    UI::UI(const VulkanMainContext& vmc, const RenderPass& render_pass, uint32_t frames) : vmc(vmc), frametime_values(plot_value_count, 0.0f), devicetimings(DeviceTimer::TIMER_COUNT, 0.0f)
    {
        for (uint32_t i = 0; i < DeviceTimer::TIMER_COUNT; ++i) devicetiming_values.push_back(FixVector<float>(plot_value_count, 0.0f));
        // use less values for plotting as the tunnel advancement happens not so often, there should still be a plot visible though
        devicetiming_values[DeviceTimer::COMPUTE_TUNNEL_ADVANCE] = FixVector<float>(128, 0.0f);

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
        ImPlot::CreateContext();

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
        ii.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&ii, render_pass.get());
        ImGui::StyleColorsDark();
    }

    void UI::self_destruct()
    {
        vmc.logical_device.get().destroyDescriptorPool(imgui_pool);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    }

    void UI::upload_font_textures(VulkanCommandContext& vcc)
    {
        vk::CommandBuffer cb = vcc.begin(vcc.graphics_cb[0]);
        ImGui_ImplVulkan_CreateFontsTexture(cb);
        vcc.submit_graphics(cb, true);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void UI::draw(vk::CommandBuffer& cb, GameState& gs)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame(vmc.window.value().get());
        ImGui::NewFrame();
        ImGui::Begin("Vulkan_Engine");
        if (ImGui::CollapsingHeader("Navigation"))
        {
            ImGui::Text("'W'A'S'D'Q'E': movement");
            ImGui::Text("Mouse_L || Arrow-Keys: panning");
            ImGui::Text("'+'-': change movement speed");
            ImGui::Text("'M': toggle mesh view");
            ImGui::Text("'N': toggle normal view");
            ImGui::Text("'T': toggle texel view");
            ImGui::Text("'C': toggle color view");
            ImGui::Text("'B': toggle player bounding box");
            ImGui::Text("'P': toggle player collision detection");
            ImGui::Text("'R': reload shaders");
            ImGui::Text("'G': Show/Hide UI");
            ImGui::Text("'F': toggle tracking camera");
            ImGui::Text("'X': toggle game mode");
            ImGui::Text("'F1': Screenshot");
            ImGui::Text("'F2': Restart game");
        }
        ImGui::Separator();
        ImGui::Checkbox("Meshview", &(gs.mesh_view));
        ImGui::SameLine();
        ImGui::Checkbox("NormalView", &(gs.normal_view));
        ImGui::SameLine();
        ImGui::Checkbox("TexelView", &(gs.tex_view));
        ImGui::Checkbox("ColorView", &(gs.color_view));
        ImGui::SameLine();
        ImGui::Checkbox("SegmentUIDView", &(gs.segment_uid_view));
        ImGui::Separator();
        ImGui::Checkbox("CollisionDetection", &(gs.collision_detection_active));
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::Text("Player pos: %.4f;%.4f;%.4f", gs.player_data.pos.x, gs.player_data.pos.y, gs.player_data.pos.z);
        ImGui::Text("Player dir: %.4f;%.4f;%.4f", gs.player_data.dir.x, gs.player_data.dir.y, gs.player_data.dir.z);
        ImGui::Text("Player up: %.4f;%.4f;%.4f", gs.player_data.up.x, gs.player_data.up.y, gs.player_data.up.z);
        ImGui::Text("Distances");
        for (const float d : gs.collision_results.distances) ImGui::Text("%.4f", d);
        ImGui::Separator();
        ImGui::Text("Score: %.2f", gs.tunnel_distance_travelled + gs.segment_distance_travelled);
        ImGui::Separator();
        time_diff = time_diff * (1 - update_weight) + gs.time_diff * update_weight;
        frametime = frametime * (1 - update_weight) + gs.frametime * update_weight;
        frametime_values.push_back(gs.frametime);
        for (uint32_t i = 0; i < DeviceTimer::TIMER_COUNT; ++i)
        {
            if (!std::signbit(gs.devicetimings[i])) 
            {
                devicetimings[i] = devicetimings[i] * (1 - update_weight) + gs.devicetimings[i] * update_weight;
                devicetiming_values[i].push_back(gs.devicetimings[i]);
            }
        }
        if (ImGui::CollapsingHeader("Timings"))
        {
            ImGui::Text("%.4f ms; FPS: %.2f (%.4f ms; FPS: %.2f)", time_diff * 1000, 1.0 / time_diff, frametime, 1000.0 / frametime);
            ImGui::Text("RENDERING_ALL: %.4f ms", devicetimings[DeviceTimer::RENDERING_ALL]);
            ImGui::Text("RENDERING_APP: %.4f ms", devicetimings[DeviceTimer::RENDERING_APP]);
            ImGui::Text("RENDERING_UI: %.4f ms", devicetimings[DeviceTimer::RENDERING_UI]);
            ImGui::Text("RENDERING_TUNNEL: %.4f ms", devicetimings[DeviceTimer::RENDERING_TUNNEL]);
            ImGui::Text("COMPUTE_TUNNEL_ADVANCE: %.4f ms", devicetimings[DeviceTimer::COMPUTE_TUNNEL_ADVANCE]);
            ImGui::Text("FIREFLY_MOVE_STEP: %.4f ms", devicetimings[DeviceTimer::FIREFLY_MOVE_STEP]);
            ImGui::Text("PLAYER_TUNNEL_COLLISION: %.4f ms", devicetimings[DeviceTimer::COMPUTE_PLAYER_TUNNEL_COLLISION]);
        }
        if (ImGui::CollapsingHeader("Plots"))
        {
            if (ImPlot::BeginPlot("Rendering Timings"))
            {
                ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0.0, 100.0);
                ImPlot::SetupAxes("Frame", "Time [ms]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit);
                ImPlot::PlotLine("FRAMETIME", frametime_values.data(), frametime_values.size());
                ImPlot::PlotLine("RENDERING_ALL", devicetiming_values[DeviceTimer::RENDERING_ALL].data(), devicetiming_values[DeviceTimer::RENDERING_ALL].size());
                ImPlot::PlotLine("RENDERING_APP", devicetiming_values[DeviceTimer::RENDERING_APP].data(), devicetiming_values[DeviceTimer::RENDERING_APP].size());
                ImPlot::PlotLine("RENDERING_UI", devicetiming_values[DeviceTimer::RENDERING_UI].data(), devicetiming_values[DeviceTimer::RENDERING_UI].size());
                ImPlot::PlotLine("RENDERING_TUNNEL", devicetiming_values[DeviceTimer::RENDERING_TUNNEL].data(), devicetiming_values[DeviceTimer::RENDERING_TUNNEL].size());
                ImPlot::PlotLine("FIREFLY_MOVE_STEP", devicetiming_values[DeviceTimer::FIREFLY_MOVE_STEP].data(), devicetiming_values[DeviceTimer::FIREFLY_MOVE_STEP].size());
                ImPlot::PlotLine("PLAYER_TUNNEL_COLLISION", devicetiming_values[DeviceTimer::COMPUTE_PLAYER_TUNNEL_COLLISION].data(), devicetiming_values[DeviceTimer::COMPUTE_PLAYER_TUNNEL_COLLISION].size());
                ImPlot::EndPlot();
            }
            if (ImPlot::BeginPlot("Compute Timings"))
            {
                ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0.0, 100.0);
                ImPlot::SetupAxes("Frame", "Time [ms]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit);
                ImPlot::PlotLine("COMPUTE_TUNNEL_ADVANCE", devicetiming_values[DeviceTimer::COMPUTE_TUNNEL_ADVANCE].data(), devicetiming_values[DeviceTimer::COMPUTE_TUNNEL_ADVANCE].size());
                ImPlot::EndPlot();
            }
        }
        ImGui::End();
        ImGui::EndFrame();

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
    }
} // namespace ve
