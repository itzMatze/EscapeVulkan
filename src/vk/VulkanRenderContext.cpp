#include "vk/VulkanRenderContext.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_sdl.h"

namespace ve
{
    VulkanRenderContext::VulkanRenderContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc, VulkanStorageContext& vsc) : vmc(vmc), vcc(vcc), vsc(vsc), swapchain(vmc), scene(vmc, vcc, vsc), ui(vmc, swapchain.get_render_pass(), frames_in_flight)
    {
        vcc.add_graphics_buffers(frames_in_flight);
        vcc.add_transfer_buffers(1);

        ui.upload_font_textures(vcc);

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            timers.emplace_back(vmc);
            sync_indices[SyncNames::SImageAvailable].push_back(vcc.sync.add_semaphore());
            sync_indices[SyncNames::SRenderFinished].push_back(vcc.sync.add_semaphore());
            sync_indices[SyncNames::FRenderFinished].push_back(vcc.sync.add_fence());
        }

        spdlog::info("Created VulkanRenderContext");
    }

    void VulkanRenderContext::self_destruct()
    {
        for (auto& timer : timers) timer.self_destruct();
        timers.clear();
        ui.self_destruct();
        scene.self_destruct();
        swapchain.self_destruct(true);
        spdlog::info("Destroyed VulkanRenderContext");
    }

    void VulkanRenderContext::load_scene(const std::string& filename)
    {
        auto t1 = std::chrono::high_resolution_clock::now();

        vcc.sync.wait_idle();
        if (scene.loaded) scene.self_destruct();
        scene.load(std::string("../assets/scenes/") + filename);

        // add one descriptor set for every frame
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            scene.add_bindings();
        }

        scene.construct(swapchain.get_render_pass());

        auto t2 = std::chrono::high_resolution_clock::now();
        spdlog::info("Loading scene took: {} ms", (std::chrono::duration<double, std::milli>(t2 - t1).count()));
    }

    void VulkanRenderContext::draw_frame(DrawInfo& di)
    {
        total_time += di.time_diff;
        //scene.rotate("Player", di.time_diff * 90.f, glm::vec3(0.0f, 1.0f, 0.0f));
        scene.rotate("floor", di.time_diff * 90.f, glm::vec3(0.0f, 1.0f, 0.0f));

        vk::ResultValue<uint32_t> image_idx = vmc.logical_device.get().acquireNextImageKHR(swapchain.get(), uint64_t(-1), vcc.sync.get_semaphore(sync_indices[SyncNames::SImageAvailable][di.current_frame]));
        VE_CHECK(image_idx.result, "Failed to acquire next image!");
        vcc.sync.wait_for_fence(sync_indices[SyncNames::FRenderFinished][di.current_frame]);
        vcc.sync.reset_fence(sync_indices[SyncNames::FRenderFinished][di.current_frame]);
        record_graphics_command_buffer(image_idx.value, di);
        submit_graphics(image_idx.value, di);
        di.current_frame = (di.current_frame + 1) % frames_in_flight;
        for (uint32_t i = 0; i < DeviceTimer::TIMER_COUNT; ++i)
        {
            double timing = timers[di.current_frame].get_result_by_idx(i);
            if (!std::signbit(timing)) di.devicetimings[i] = timing;
        }
    }

    vk::Extent2D VulkanRenderContext::recreate_swapchain()
    {
        vcc.sync.wait_idle();
        swapchain.self_destruct(false);
        swapchain.create();
        return swapchain.get_extent();
    }

    void VulkanRenderContext::record_graphics_command_buffer(uint32_t image_idx, DrawInfo& di)
    {
        vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[di.current_frame]);
        timers[di.current_frame].reset_all(cb);
        timers[di.current_frame].start(cb, DeviceTimer::ALL_RENDERING, vk::PipelineStageFlagBits::eAllCommands);
        vk::RenderPassBeginInfo rpbi{};
        rpbi.sType = vk::StructureType::eRenderPassBeginInfo;
        rpbi.renderPass = swapchain.get_render_pass().get();
        rpbi.framebuffer = swapchain.get_framebuffer(image_idx);
        rpbi.renderArea.offset = vk::Offset2D(0, 0);
        rpbi.renderArea.extent = swapchain.get_extent();
        std::vector<vk::ClearValue> clear_values(2);
        clear_values[0].color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
        clear_values[1].depthStencil.depth = 1.0f;
        clear_values[1].depthStencil.stencil = 0;
        if (swapchain.get_render_pass().get_sample_count() != vk::SampleCountFlagBits::e1) clear_values.push_back(vk::ClearValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));
        rpbi.clearValueCount = clear_values.size();
        rpbi.pClearValues = clear_values.data();
        cb.beginRenderPass(rpbi, vk::SubpassContents::eInline);

        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = swapchain.get_extent().width;
        viewport.height = swapchain.get_extent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cb.setViewport(0, viewport);
        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D(0, 0);
        scissor.extent = swapchain.get_extent();
        cb.setScissor(0, scissor);

        std::vector<vk::DeviceSize> offsets(1, 0);

        timers[di.current_frame].start(cb, DeviceTimer::APP_RENDERING, vk::PipelineStageFlagBits::eAllCommands);
        scene.draw(cb, di);
        timers[di.current_frame].stop(cb, DeviceTimer::APP_RENDERING, vk::PipelineStageFlagBits::eAllCommands);
        timers[di.current_frame].start(cb, DeviceTimer::UI_RENDERING, vk::PipelineStageFlagBits::eAllCommands);
        if (di.show_ui) ui.draw(cb, di);
        timers[di.current_frame].stop(cb, DeviceTimer::UI_RENDERING, vk::PipelineStageFlagBits::eAllCommands);

        cb.endRenderPass();
        timers[di.current_frame].stop(cb, DeviceTimer::ALL_RENDERING, vk::PipelineStageFlagBits::eAllCommands);
        cb.end();
    }

    void VulkanRenderContext::submit_graphics(uint32_t image_idx, DrawInfo& di)
    {
        vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo si{};
        si.sType = vk::StructureType::eSubmitInfo;
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = &vcc.sync.get_semaphore(sync_indices[SyncNames::SImageAvailable][di.current_frame]);
        si.pWaitDstStageMask = &wait_stage;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &vcc.graphics_cb[di.current_frame];
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = &vcc.sync.get_semaphore(sync_indices[SyncNames::SRenderFinished][di.current_frame]);
        vmc.get_graphics_queue().submit(si, vcc.sync.get_fence(sync_indices[SyncNames::FRenderFinished][di.current_frame]));

        vk::PresentInfoKHR present_info{};
        present_info.sType = vk::StructureType::ePresentInfoKHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &vcc.sync.get_semaphore(sync_indices[SyncNames::SRenderFinished][di.current_frame]);
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain.get();
        present_info.pImageIndices = &image_idx;
        present_info.pResults = nullptr;
        VE_CHECK(vmc.get_present_queue().presentKHR(present_info), "Failed to present image!");
    }
} // namespace ve
