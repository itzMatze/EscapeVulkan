#include "WorkContext.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_sdl.h"

namespace ve
{
    constexpr uint32_t frames_in_flight = 2;

    WorkContext::WorkContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc) : vmc(vmc), vcc(vcc), storage(vmc, vcc), swapchain(vmc), scene(vmc, vcc, storage), ui(vmc, swapchain.get_render_pass(), frames_in_flight), co(vmc)
    {
        vcc.add_graphics_buffers(frames_in_flight);
        vcc.add_compute_buffers(frames_in_flight);
        vcc.add_transfer_buffers(1);

        ui.upload_font_textures(vcc);

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            timers.emplace_back(vmc);
            syncs.emplace_back(vmc.logical_device.get());
        }

        spdlog::info("Created WorkContext");
    }

    void WorkContext::self_destruct()
    {
        for (auto& sync : syncs) sync.self_destruct();
        syncs.clear();
        for (auto& timer : timers) timer.self_destruct();
        timers.clear();
        ui.self_destruct();
        scene.self_destruct();
        swapchain.self_destruct(true);
        spdlog::info("Destroyed WorkContext");
    }

    void WorkContext::load_scene(const std::string& filename)
    {
        HostTimer timer;
        syncs[0].wait_idle();
        if (scene.loaded) scene.self_destruct();
        scene.load(std::string("../assets/scenes/") + filename);
        scene.construct(swapchain.get_render_pass(), frames_in_flight);
        spdlog::info("Loading scene took: {} ms", (timer.elapsed()));
    }

    void WorkContext::draw_frame(DrawInfo& di)
    {
        total_time += di.time_diff;
        //scene.rotate("Player", di.time_diff * 90.f, glm::vec3(0.0f, 1.0f, 0.0f));
        scene.rotate("floor", di.time_diff * 90.f, glm::vec3(0.0f, 1.0f, 0.0f));

        vk::ResultValue<uint32_t> image_idx = vmc.logical_device.get().acquireNextImageKHR(swapchain.get(), uint64_t(-1), syncs[di.current_frame].get_semaphore(Synchronization::S_IMAGE_AVAILABLE));
        VE_CHECK(image_idx.result, "Failed to acquire next image!");
        syncs[di.current_frame].wait_for_fence(Synchronization::F_RENDER_FINISHED);
        syncs[di.current_frame].reset_fence(Synchronization::F_RENDER_FINISHED);
        record_graphics_command_buffer(image_idx.value, di);
        submit_graphics(image_idx.value, di);
        di.current_frame = (di.current_frame + 1) % frames_in_flight;
        for (uint32_t i = 0; i < DeviceTimer::TIMER_COUNT; ++i)
        {
            double timing = timers[di.current_frame].get_result_by_idx(i);
            if (!std::signbit(timing)) di.devicetimings[i] = timing;
        }
    }

    vk::Extent2D WorkContext::recreate_swapchain()
    {
        syncs[0].wait_idle();
        swapchain.self_destruct(false);
        swapchain.create();
        return swapchain.get_extent();
    }

    void WorkContext::record_graphics_command_buffer(uint32_t image_idx, DrawInfo& di)
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

    void WorkContext::submit_graphics(uint32_t image_idx, DrawInfo& di)
    {
        vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo si{};
        si.sType = vk::StructureType::eSubmitInfo;
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = &syncs[di.current_frame].get_semaphore(Synchronization::S_IMAGE_AVAILABLE);
        si.pWaitDstStageMask = &wait_stage;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &vcc.graphics_cb[di.current_frame];
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = &syncs[di.current_frame].get_semaphore(Synchronization::S_RENDER_FINISHED);
        vmc.get_graphics_queue().submit(si, syncs[di.current_frame].get_fence(Synchronization::F_RENDER_FINISHED));

        vk::PresentInfoKHR present_info{};
        present_info.sType = vk::StructureType::ePresentInfoKHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &syncs[di.current_frame].get_semaphore(Synchronization::S_RENDER_FINISHED);
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain.get();
        present_info.pImageIndices = &image_idx;
        present_info.pResults = nullptr;
        VE_CHECK(vmc.get_present_queue().presentKHR(present_info), "Failed to present image!");
    }
} // namespace ve
