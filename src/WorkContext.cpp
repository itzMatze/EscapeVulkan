#include "WorkContext.hpp"
#include "vk/TunnelConstants.hpp"

namespace ve
{
WorkContext::WorkContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc) : vmc(vmc), vcc(vcc), storage(vmc, vcc), swapchain(vmc, vcc, storage), scene(vmc, vcc, storage), ui(vmc, swapchain.get_render_pass(), frames_in_flight), lighting(vmc, storage)
{
    vcc.add_graphics_buffers(frames_in_flight * 3);
    vcc.add_compute_buffers(frames_in_flight * 3);
    vcc.add_transfer_buffers(1);

    swapchain.construct();

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
    lighting.self_destruct();
    spdlog::info("Destroyed WorkContext");
}

void WorkContext::reload_shaders()
{
    vmc.logical_device.get().waitIdle();
    scene.reload_shaders(swapchain.get_deferred_render_pass());
    // lighting.reload_shaders();
}

void WorkContext::load_scene(const std::string& filename)
{
    HostTimer timer;
    vmc.logical_device.get().waitIdle();
    if (scene.loaded)
    {
        scene.self_destruct();
    }
    scene.load(std::string("../assets/scenes/") + filename);
    scene.construct(swapchain.get_deferred_render_pass());
    spdlog::info("Loading scene took: {} ms", (timer.elapsed()));
    lighting.construct(scene.get_light_count(), swapchain);
}

void WorkContext::restart()
{
    vmc.logical_device.get().waitIdle();
    scene.restart();
}

void WorkContext::draw_frame(GameState& gs)
{
    //scene.rotate("Player", gs.time_diff * 90.f, glm::vec3(0.0f, 1.0f, 0.0f));

    syncs[gs.game_data.current_frame].wait_for_fence(Synchronization::F_RENDER_FINISHED);
    syncs[gs.game_data.current_frame].reset_fence(Synchronization::F_RENDER_FINISHED);
    vk::ResultValue<uint32_t> image_idx = vmc.logical_device.get().acquireNextImageKHR(swapchain.get(), uint64_t(-1), syncs[gs.game_data.current_frame].get_semaphore(Synchronization::S_IMAGE_AVAILABLE));
    VE_CHECK(image_idx.result, "Failed to acquire next image!");
    for (uint32_t i = 0; i < DeviceTimer::TIMER_COUNT && gs.game_data.total_frames >= timers.size(); ++i)
    {
        double timing = timers[gs.game_data.current_frame].get_result_by_idx(i);
        gs.session_data.devicetimings[i] = timing;
    }
    if (gs.settings.save_screenshot)
    {
        swapchain.save_screenshot(vcc, image_idx.value, gs.game_data.current_frame);
        gs.settings.save_screenshot = false;
    }
    record_graphics_command_buffer(image_idx.value, gs);
    submit(image_idx.value, gs);
    gs.game_data.current_frame = (gs.game_data.current_frame + 1) % frames_in_flight;
}

vk::Extent2D WorkContext::recreate_swapchain()
{
    vmc.logical_device.get().waitIdle();
    swapchain.self_destruct(false);
    swapchain.construct();
    lighting.self_destruct();
    lighting.construct(scene.get_light_count(), swapchain);
    return swapchain.get_extent();
}

void WorkContext::record_graphics_command_buffer(uint32_t image_idx, GameState& gs)
{
    vk::CommandBuffer& compute_cb = vcc.begin(vcc.compute_cb[gs.game_data.current_frame + frames_in_flight * 2]);
    scene.update_game_state(compute_cb, gs, timers[gs.game_data.current_frame]);
    compute_cb.end();

    vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[gs.game_data.current_frame]);
    timers[gs.game_data.current_frame].reset(cb, {DeviceTimer::RENDERING_ALL, DeviceTimer::RENDERING_APP, DeviceTimer::RENDERING_UI, DeviceTimer::RENDERING_TUNNEL});
    timers[gs.game_data.current_frame].start(cb, DeviceTimer::RENDERING_ALL, vk::PipelineStageFlagBits::eAllGraphics);
    vk::RenderPassBeginInfo rpbi{};
    rpbi.sType = vk::StructureType::eRenderPassBeginInfo;
    rpbi.renderPass = swapchain.get_deferred_render_pass().get();
    rpbi.framebuffer = swapchain.get_deferred_framebuffer();
    rpbi.renderArea.offset = vk::Offset2D(0, 0);
    rpbi.renderArea.extent = swapchain.get_extent();
    std::vector<vk::ClearValue> clear_values(6);
    clear_values[0].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    clear_values[1].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    clear_values[2].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    clear_values[3].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    clear_values[4].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
    clear_values[5].depthStencil.depth = 1.0f;
    clear_values[5].depthStencil.stencil = 0;
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

    if (!gs.settings.disable_rendering)
    {
        timers[gs.game_data.current_frame].start(cb, DeviceTimer::RENDERING_APP, vk::PipelineStageFlagBits::eTopOfPipe);
        scene.draw(cb, gs, timers[gs.game_data.current_frame]);
        timers[gs.game_data.current_frame].stop(cb, DeviceTimer::RENDERING_APP, vk::PipelineStageFlagBits::eBottomOfPipe);
    }

    cb.endRenderPass();
    cb.end();
    vk::CommandBuffer& lighting_cb_0 = vcc.begin(vcc.graphics_cb[gs.game_data.current_frame + frames_in_flight]);
    vk::RenderPassBeginInfo lighting_rpbi{};
    lighting_rpbi.sType = vk::StructureType::eRenderPassBeginInfo;
    lighting_rpbi.renderPass = swapchain.get_render_pass().get();
    lighting_rpbi.framebuffer = swapchain.get_framebuffer(image_idx);
    lighting_rpbi.renderArea.offset = vk::Offset2D(0, 0);
    lighting_rpbi.renderArea.extent = swapchain.get_extent();
    std::vector<vk::ClearValue> lighting_clear_values(2);
    lighting_clear_values[0].color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
    lighting_clear_values[1].depthStencil.depth = 1.0f;
    lighting_clear_values[1].depthStencil.stencil = 0;
    lighting_rpbi.clearValueCount = lighting_clear_values.size();
    lighting_rpbi.pClearValues = lighting_clear_values.data();
    lighting_cb_0.beginRenderPass(lighting_rpbi, vk::SubpassContents::eInline);
    lighting_cb_0.setViewport(0, viewport);
    lighting_cb_0.setScissor(0, scissor);
    lighting.pre_pass(lighting_cb_0, gs);
    lighting_cb_0.endRenderPass();
    lighting_cb_0.end();

    vk::CommandBuffer& lighting_cb_1 = vcc.begin(vcc.graphics_cb[gs.game_data.current_frame + frames_in_flight * 2]);
    lighting_cb_1.beginRenderPass(lighting_rpbi, vk::SubpassContents::eInline);
    lighting_cb_1.setViewport(0, viewport);
    lighting_cb_1.setScissor(0, scissor);
    lighting.main_pass(lighting_cb_1, gs);
    timers[gs.game_data.current_frame].start(lighting_cb_1, DeviceTimer::RENDERING_UI, vk::PipelineStageFlagBits::eTopOfPipe);
    if (gs.settings.show_ui) ui.draw(lighting_cb_1, gs);
    timers[gs.game_data.current_frame].stop(lighting_cb_1, DeviceTimer::RENDERING_UI, vk::PipelineStageFlagBits::eBottomOfPipe);
    lighting_cb_1.endRenderPass();
    timers[gs.game_data.current_frame].stop(lighting_cb_1 , DeviceTimer::RENDERING_ALL, vk::PipelineStageFlagBits::eAllGraphics);
    lighting_cb_1.end();
}

void WorkContext::submit(uint32_t image_idx, GameState& gs)
{
    std::array<vk::CommandBuffer, 3> compute_cbs{vcc.compute_cb[gs.game_data.current_frame], vcc.compute_cb[gs.game_data.current_frame + frames_in_flight], vcc.compute_cb[gs.game_data.current_frame + frames_in_flight * 2]};
    vk::SubmitInfo compute_si{};
    compute_si.sType = vk::StructureType::eSubmitInfo;
    compute_si.waitSemaphoreCount = 0;
    compute_si.commandBufferCount = compute_cbs.size();
    compute_si.pCommandBuffers = compute_cbs.data();
    compute_si.signalSemaphoreCount = 1;
    compute_si.pSignalSemaphores = &syncs[gs.game_data.current_frame].get_semaphore(Synchronization::S_COMPUTE_FINISHED);
    vmc.get_compute_queue().submit(compute_si);

    std::vector<vk::SubmitInfo> render_si(3);
    std::vector<vk::PipelineStageFlags> geometry_pass_wait_stages;
    std::vector<vk::Semaphore> geometry_pass_wait_semaphores;
    geometry_pass_wait_stages.push_back(vk::PipelineStageFlagBits::eVertexInput);
    geometry_pass_wait_semaphores.push_back(syncs[gs.game_data.current_frame].get_semaphore(Synchronization::S_COMPUTE_FINISHED));
    render_si[0].sType = vk::StructureType::eSubmitInfo;
    render_si[0].waitSemaphoreCount = geometry_pass_wait_semaphores.size();
    render_si[0].pWaitSemaphores = geometry_pass_wait_semaphores.data();
    render_si[0].pWaitDstStageMask = geometry_pass_wait_stages.data();
    render_si[0].commandBufferCount = 1;
    render_si[0].pCommandBuffers = &vcc.graphics_cb[gs.game_data.current_frame];
    render_si[0].signalSemaphoreCount = 1;
    render_si[0].pSignalSemaphores = &syncs[gs.game_data.current_frame].get_semaphore(Synchronization::S_GEOMETRY_PASS_FINISHED);

    std::vector<vk::PipelineStageFlags> lighting_pass_0_wait_stages;
    std::vector<vk::Semaphore> lighting_pass_0_wait_semaphores;
    lighting_pass_0_wait_stages.push_back(vk::PipelineStageFlagBits::eFragmentShader);
    lighting_pass_0_wait_semaphores.push_back(syncs[gs.game_data.current_frame].get_semaphore(Synchronization::S_GEOMETRY_PASS_FINISHED));
    lighting_pass_0_wait_stages.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    lighting_pass_0_wait_semaphores.push_back(syncs[gs.game_data.current_frame].get_semaphore(Synchronization::S_IMAGE_AVAILABLE));
    render_si[1].sType = vk::StructureType::eSubmitInfo;
    render_si[1].waitSemaphoreCount = lighting_pass_0_wait_semaphores.size();
    render_si[1].pWaitSemaphores = lighting_pass_0_wait_semaphores.data();
    render_si[1].pWaitDstStageMask = lighting_pass_0_wait_stages.data();
    render_si[1].commandBufferCount = 1;
    render_si[1].pCommandBuffers = &vcc.graphics_cb[gs.game_data.current_frame + frames_in_flight];
    render_si[1].signalSemaphoreCount = 1;
    render_si[1].pSignalSemaphores = &syncs[gs.game_data.current_frame].get_semaphore(Synchronization::S_LIGHTING_PASS_0_FINISHED);

    std::vector<vk::PipelineStageFlags> lighting_pass_1_wait_stages;
    std::vector<vk::Semaphore> lighting_pass_1_wait_semaphores;
    lighting_pass_1_wait_stages.push_back(vk::PipelineStageFlagBits::eFragmentShader);
    lighting_pass_1_wait_semaphores.push_back(syncs[gs.game_data.current_frame].get_semaphore(Synchronization::S_LIGHTING_PASS_0_FINISHED));
    render_si[2].sType = vk::StructureType::eSubmitInfo;
    render_si[2].waitSemaphoreCount = lighting_pass_1_wait_semaphores.size();
    render_si[2].pWaitSemaphores = lighting_pass_1_wait_semaphores.data();
    render_si[2].pWaitDstStageMask = lighting_pass_1_wait_stages.data();
    render_si[2].commandBufferCount = 1;
    render_si[2].pCommandBuffers = &vcc.graphics_cb[gs.game_data.current_frame + frames_in_flight * 2];
    render_si[2].signalSemaphoreCount = 1;
    render_si[2].pSignalSemaphores = &syncs[gs.game_data.current_frame].get_semaphore(Synchronization::S_LIGHTING_PASS_1_FINISHED);
    vmc.get_graphics_queue().submit(render_si, syncs[gs.game_data.current_frame].get_fence(Synchronization::F_RENDER_FINISHED));

    vk::PresentInfoKHR present_info{};
    present_info.sType = vk::StructureType::ePresentInfoKHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &syncs[gs.game_data.current_frame].get_semaphore(Synchronization::S_LIGHTING_PASS_1_FINISHED);
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain.get();
    present_info.pImageIndices = &image_idx;
    present_info.pResults = nullptr;
    VE_CHECK(vmc.get_present_queue().presentKHR(present_info), "Failed to present image!");
}
} // namespace ve
