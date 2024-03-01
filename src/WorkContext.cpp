#include "WorkContext.hpp"

#include <thread>

namespace ve
{
    WorkContext::WorkContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc) : vmc(vmc), vcc(vcc), storage(vmc, vcc), swapchain(vmc, vcc, storage), scene(vmc, vcc, storage), ui(vmc, swapchain.get_render_pass(), frames_in_flight), lighting_pipeline_0(vmc), lighting_pipeline_1(vmc), lighting_dsh(vmc)
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
        lighting_pipeline_0.self_destruct();
        lighting_pipeline_1.self_destruct();
        lighting_dsh.self_destruct();
        for (uint32_t i : restir_reservoir_buffers) storage.destroy_buffer(i);
        restir_reservoir_buffers.clear();
        spdlog::info("Destroyed WorkContext");
    }

    void WorkContext::reload_shaders()
    {
        vmc.logical_device.get().waitIdle();
        scene.reload_shaders(swapchain.get_deferred_render_pass());
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
        create_lighting_pipeline();
    }

    void WorkContext::create_lighting_pipeline()
    {
        create_lighting_descriptor_sets();
        std::vector<ShaderInfo> shader_infos(2);
        std::array<vk::SpecializationMapEntry, 7> fragment_entries;
        fragment_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
        fragment_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
        fragment_entries[2] = vk::SpecializationMapEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t));
        fragment_entries[3] = vk::SpecializationMapEntry(3, sizeof(uint32_t) * 3, sizeof(uint32_t));
        fragment_entries[4] = vk::SpecializationMapEntry(4, sizeof(uint32_t) * 4, sizeof(uint32_t));
        fragment_entries[5] = vk::SpecializationMapEntry(5, sizeof(uint32_t) * 5, sizeof(uint32_t));
        fragment_entries[6] = vk::SpecializationMapEntry(6, sizeof(uint32_t) * 6, sizeof(uint32_t));
        std::array<uint32_t, 7> fragment_entries_data{scene.get_light_count(), segment_count, fireflies_per_segment, reservoir_count, swapchain.get_extent().width, swapchain.get_extent().height, 1};
        vk::SpecializationInfo fragment_spec_info(fragment_entries.size(), fragment_entries.data(), sizeof(uint32_t) * fragment_entries_data.size(), fragment_entries_data.data());

        shader_infos[0] = ShaderInfo{"lighting.vert", vk::ShaderStageFlagBits::eVertex};
        shader_infos[1] = ShaderInfo{"lighting.frag", vk::ShaderStageFlagBits::eFragment, fragment_spec_info};
        lighting_pipeline_0.construct(swapchain.get_render_pass(), lighting_dsh.get_layouts()[0], shader_infos, vk::PolygonMode::eFill, std::vector<vk::VertexInputBindingDescription>(), std::vector<vk::VertexInputAttributeDescription>(), vk::PrimitiveTopology::eTriangleList, {vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0, sizeof(LightingPassPushConstants))});

        fragment_entries_data[6] = 0;
        shader_infos[1] = ShaderInfo{"lighting.frag", vk::ShaderStageFlagBits::eFragment, fragment_spec_info};
        lighting_pipeline_1.construct(swapchain.get_render_pass(), lighting_dsh.get_layouts()[0], shader_infos, vk::PolygonMode::eFill, std::vector<vk::VertexInputBindingDescription>(), std::vector<vk::VertexInputAttributeDescription>(), vk::PrimitiveTopology::eTriangleList, {vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0, sizeof(LightingPassPushConstants))});
    }

    void WorkContext::create_lighting_descriptor_sets()
    {
        std::vector<Reservoir> reservoirs(swapchain.get_extent().width * swapchain.get_extent().height * reservoir_count);
        for(uint32_t i = 0; i < frames_in_flight; ++i)
        {
            restir_reservoir_buffers.push_back(storage.add_buffer(reservoirs, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.graphics));
        }
        lighting_dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(5, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(6, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(10, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(11, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(12, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(13, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(99, vk::DescriptorType::eAccelerationStructureKHR, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(100, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(101, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(102, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(103, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(104, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(200, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
        lighting_dsh.add_binding(201, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);

        for (uint32_t j = 0; j < frames_in_flight; ++j)
        {
            for (uint32_t i = 0; i < frames_in_flight; ++i)
            {
                lighting_dsh.new_set();
                lighting_dsh.add_descriptor(1, storage.get_buffer_by_name("mesh_render_data"));
                lighting_dsh.add_descriptor(2, storage.get_image_by_name("textures"));
                lighting_dsh.add_descriptor(3, storage.get_buffer_by_name("materials"));
                lighting_dsh.add_descriptor(4, storage.get_buffer_by_name("spaceship_lights_" + std::to_string(j)));
                lighting_dsh.add_descriptor(5, storage.get_buffer_by_name("firefly_vertices_" + std::to_string(j)));
                lighting_dsh.add_descriptor(6, storage.get_image_by_name("noise_textures"));
                lighting_dsh.add_descriptor(10, storage.get_buffer_by_name("tunnel_indices"));
                lighting_dsh.add_descriptor(11, storage.get_buffer_by_name("tunnel_vertices"));
                lighting_dsh.add_descriptor(12, storage.get_buffer_by_name("indices"));
                lighting_dsh.add_descriptor(13, storage.get_buffer_by_name("vertices"));
                lighting_dsh.add_descriptor(99, storage.get_buffer_by_name("tlas_" + std::to_string(j)));
                lighting_dsh.add_descriptor(100, storage.get_image_by_name("deferred_position"));
                lighting_dsh.add_descriptor(101, storage.get_image_by_name("deferred_normal"));
                lighting_dsh.add_descriptor(102, storage.get_image_by_name("deferred_color"));
                lighting_dsh.add_descriptor(103, storage.get_image_by_name("deferred_segment_uid"));
                lighting_dsh.add_descriptor(104, storage.get_image_by_name("deferred_motion"));
                lighting_dsh.add_descriptor(200, storage.get_buffer(restir_reservoir_buffers[1 - i]));
                lighting_dsh.add_descriptor(201, storage.get_buffer(restir_reservoir_buffers[i]));
            }
        }
        lighting_dsh.construct();
    }

    void WorkContext::restart()
    {
        vmc.logical_device.get().waitIdle();
        scene.restart();
    }

    void WorkContext::draw_frame(GameState& gs)
    {
        //scene.rotate("Player", gs.time_diff * 90.f, glm::vec3(0.0f, 1.0f, 0.0f));

        syncs[gs.current_frame].wait_for_fence(Synchronization::F_RENDER_FINISHED);
        syncs[gs.current_frame].reset_fence(Synchronization::F_RENDER_FINISHED);
        vk::ResultValue<uint32_t> image_idx = vmc.logical_device.get().acquireNextImageKHR(swapchain.get(), uint64_t(-1), syncs[gs.current_frame].get_semaphore(Synchronization::S_IMAGE_AVAILABLE));
        VE_CHECK(image_idx.result, "Failed to acquire next image!");
        for (uint32_t i = 0; i < DeviceTimer::TIMER_COUNT && gs.total_frames >= timers.size(); ++i)
        {
            double timing = timers[gs.current_frame].get_result_by_idx(i);
            gs.devicetimings[i] = timing;
        }
        if (gs.save_screenshot)
        {
            swapchain.save_screenshot(vcc, image_idx.value, gs.current_frame);
            gs.save_screenshot = false;
        }
        if (gs.player_lifes == 0)
        {
            display_dead_screen(image_idx.value, gs);
            std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(5.0f));
        }
        else
        {
            record_graphics_command_buffer(image_idx.value, gs);
            submit(image_idx.value, gs);
        }
        gs.current_frame = (gs.current_frame + 1) % frames_in_flight;
    }

    vk::Extent2D WorkContext::recreate_swapchain()
    {
        vmc.logical_device.get().waitIdle();
        swapchain.self_destruct(false);
        swapchain.construct();
        lighting_dsh.self_destruct();
        for (uint32_t i : restir_reservoir_buffers) storage.destroy_buffer(i);
        lighting_pipeline_0.self_destruct();
        lighting_pipeline_1.self_destruct();
        restir_reservoir_buffers.clear();
        create_lighting_pipeline();
        return swapchain.get_extent();
    }

    void WorkContext::display_dead_screen(uint32_t image_idx, GameState& gs)
    {
        vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[gs.current_frame + frames_in_flight * 2]);
        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = swapchain.get_extent().width;
        viewport.height = swapchain.get_extent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D(0, 0);
        scissor.extent = swapchain.get_extent();
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
        cb.beginRenderPass(lighting_rpbi, vk::SubpassContents::eInline);
        cb.setViewport(0, viewport);
        cb.setScissor(0, scissor);
        if (gs.show_ui) ui.draw(cb, gs);
        cb.endRenderPass();
        cb.end();
        std::vector<vk::PipelineStageFlags> wait_stages;
        std::vector<vk::Semaphore> wait_semaphores;
        wait_stages.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        wait_semaphores.push_back(syncs[gs.current_frame].get_semaphore(Synchronization::S_IMAGE_AVAILABLE));
        vk::SubmitInfo submit_info;
        submit_info.sType = vk::StructureType::eSubmitInfo;
        submit_info.waitSemaphoreCount = wait_semaphores.size();
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &vcc.graphics_cb[gs.current_frame + frames_in_flight * 2];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &syncs[gs.current_frame].get_semaphore(Synchronization::S_LIGHTING_PASS_1_FINISHED);
        vmc.get_graphics_queue().submit(submit_info, syncs[gs.current_frame].get_fence(Synchronization::F_RENDER_FINISHED));

        vk::PresentInfoKHR present_info{};
        present_info.sType = vk::StructureType::ePresentInfoKHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &syncs[gs.current_frame].get_semaphore(Synchronization::S_LIGHTING_PASS_1_FINISHED);
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain.get();
        present_info.pImageIndices = &image_idx;
        present_info.pResults = nullptr;
        VE_CHECK(vmc.get_present_queue().presentKHR(present_info), "Failed to present image!");
    }

    void WorkContext::record_graphics_command_buffer(uint32_t image_idx, GameState& gs)
    {
        vk::CommandBuffer& compute_cb = vcc.begin(vcc.compute_cb[gs.current_frame + frames_in_flight * 2]);
        scene.update_game_state(compute_cb, gs, timers[gs.current_frame]);
        compute_cb.end();

        vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[gs.current_frame]);
        timers[gs.current_frame].reset(cb, {DeviceTimer::RENDERING_ALL, DeviceTimer::RENDERING_APP, DeviceTimer::RENDERING_UI, DeviceTimer::RENDERING_TUNNEL});
        timers[gs.current_frame].start(cb, DeviceTimer::RENDERING_ALL, vk::PipelineStageFlagBits::eAllGraphics);
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

        if (!gs.disable_rendering)
        {
            timers[gs.current_frame].start(cb, DeviceTimer::RENDERING_APP, vk::PipelineStageFlagBits::eTopOfPipe);
            scene.draw(cb, gs, timers[gs.current_frame]);
            timers[gs.current_frame].stop(cb, DeviceTimer::RENDERING_APP, vk::PipelineStageFlagBits::eBottomOfPipe);
        }

        cb.endRenderPass();
        cb.end();
        vk::CommandBuffer& lighting_cb_0 = vcc.begin(vcc.graphics_cb[gs.current_frame + frames_in_flight]);
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
        lighting_cb_0.bindPipeline(vk::PipelineBindPoint::eGraphics, lighting_pipeline_0.get());
        lighting_cb_0.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, lighting_pipeline_0.get_layout(), 0, lighting_dsh.get_sets()[gs.current_frame * frames_in_flight], {});
        LightingPassPushConstants lppc{.first_segment_indices_idx = gs.first_segment_indices_idx, .time = gs.time, .normal_view = gs.normal_view, .color_view = gs.color_view, .segment_uid_view = gs.segment_uid_view};
        lighting_cb_0.pushConstants(lighting_pipeline_0.get_layout(), vk::ShaderStageFlagBits::eFragment, 0, sizeof(LightingPassPushConstants), &lppc);
        if (!gs.disable_rendering)
        {
            lighting_cb_0.draw(3, 1, 0, 0);
        }
        lighting_cb_0.endRenderPass();
        lighting_cb_0.end();

        vk::CommandBuffer& lighting_cb_1 = vcc.begin(vcc.graphics_cb[gs.current_frame + frames_in_flight * 2]);
        lighting_cb_1.beginRenderPass(lighting_rpbi, vk::SubpassContents::eInline);
        lighting_cb_1.setViewport(0, viewport);
        lighting_cb_1.setScissor(0, scissor);
        lighting_cb_1.bindPipeline(vk::PipelineBindPoint::eGraphics, lighting_pipeline_1.get());
        lighting_cb_1.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, lighting_pipeline_1.get_layout(), 0, lighting_dsh.get_sets()[gs.current_frame * frames_in_flight + 1], {});
        lighting_cb_1.pushConstants(lighting_pipeline_1.get_layout(), vk::ShaderStageFlagBits::eFragment, 0, sizeof(LightingPassPushConstants), &lppc);
        if (!gs.disable_rendering)
        {
            lighting_cb_1.draw(3, 1, 0, 0);
        }
        timers[gs.current_frame].start(lighting_cb_1, DeviceTimer::RENDERING_UI, vk::PipelineStageFlagBits::eTopOfPipe);
        if (gs.show_ui) ui.draw(lighting_cb_1, gs);
        timers[gs.current_frame].stop(lighting_cb_1, DeviceTimer::RENDERING_UI, vk::PipelineStageFlagBits::eBottomOfPipe);
        lighting_cb_1.endRenderPass();
        timers[gs.current_frame].stop(lighting_cb_1 , DeviceTimer::RENDERING_ALL, vk::PipelineStageFlagBits::eAllGraphics);
        lighting_cb_1.end();
    }

    void WorkContext::submit(uint32_t image_idx, GameState& gs)
    {
        std::array<vk::CommandBuffer, 3> compute_cbs{vcc.compute_cb[gs.current_frame], vcc.compute_cb[gs.current_frame + frames_in_flight], vcc.compute_cb[gs.current_frame + frames_in_flight * 2]};
        vk::SubmitInfo compute_si{};
        compute_si.sType = vk::StructureType::eSubmitInfo;
        compute_si.waitSemaphoreCount = 0;
        compute_si.commandBufferCount = compute_cbs.size();
        compute_si.pCommandBuffers = compute_cbs.data();
        compute_si.signalSemaphoreCount = 1;
        compute_si.pSignalSemaphores = &syncs[gs.current_frame].get_semaphore(Synchronization::S_COMPUTE_FINISHED);
        vmc.get_compute_queue().submit(compute_si);

        std::vector<vk::SubmitInfo> render_si(3);
        std::vector<vk::PipelineStageFlags> geometry_pass_wait_stages;
        std::vector<vk::Semaphore> geometry_pass_wait_semaphores;
        geometry_pass_wait_stages.push_back(vk::PipelineStageFlagBits::eVertexInput);
        geometry_pass_wait_semaphores.push_back(syncs[gs.current_frame].get_semaphore(Synchronization::S_COMPUTE_FINISHED));
        render_si[0].sType = vk::StructureType::eSubmitInfo;
        render_si[0].waitSemaphoreCount = geometry_pass_wait_semaphores.size();
        render_si[0].pWaitSemaphores = geometry_pass_wait_semaphores.data();
        render_si[0].pWaitDstStageMask = geometry_pass_wait_stages.data();
        render_si[0].commandBufferCount = 1;
        render_si[0].pCommandBuffers = &vcc.graphics_cb[gs.current_frame];
        render_si[0].signalSemaphoreCount = 1;
        render_si[0].pSignalSemaphores = &syncs[gs.current_frame].get_semaphore(Synchronization::S_GEOMETRY_PASS_FINISHED);

        std::vector<vk::PipelineStageFlags> lighting_pass_0_wait_stages;
        std::vector<vk::Semaphore> lighting_pass_0_wait_semaphores;
        lighting_pass_0_wait_stages.push_back(vk::PipelineStageFlagBits::eFragmentShader);
        lighting_pass_0_wait_semaphores.push_back(syncs[gs.current_frame].get_semaphore(Synchronization::S_GEOMETRY_PASS_FINISHED));
        lighting_pass_0_wait_stages.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        lighting_pass_0_wait_semaphores.push_back(syncs[gs.current_frame].get_semaphore(Synchronization::S_IMAGE_AVAILABLE));
        render_si[1].sType = vk::StructureType::eSubmitInfo;
        render_si[1].waitSemaphoreCount = lighting_pass_0_wait_semaphores.size();
        render_si[1].pWaitSemaphores = lighting_pass_0_wait_semaphores.data();
        render_si[1].pWaitDstStageMask = lighting_pass_0_wait_stages.data();
        render_si[1].commandBufferCount = 1;
        render_si[1].pCommandBuffers = &vcc.graphics_cb[gs.current_frame + frames_in_flight];
        render_si[1].signalSemaphoreCount = 1;
        render_si[1].pSignalSemaphores = &syncs[gs.current_frame].get_semaphore(Synchronization::S_LIGHTING_PASS_0_FINISHED);

        std::vector<vk::PipelineStageFlags> lighting_pass_1_wait_stages;
        std::vector<vk::Semaphore> lighting_pass_1_wait_semaphores;
        lighting_pass_1_wait_stages.push_back(vk::PipelineStageFlagBits::eFragmentShader);
        lighting_pass_1_wait_semaphores.push_back(syncs[gs.current_frame].get_semaphore(Synchronization::S_LIGHTING_PASS_0_FINISHED));
        render_si[2].sType = vk::StructureType::eSubmitInfo;
        render_si[2].waitSemaphoreCount = lighting_pass_1_wait_semaphores.size();
        render_si[2].pWaitSemaphores = lighting_pass_1_wait_semaphores.data();
        render_si[2].pWaitDstStageMask = lighting_pass_1_wait_stages.data();
        render_si[2].commandBufferCount = 1;
        render_si[2].pCommandBuffers = &vcc.graphics_cb[gs.current_frame + frames_in_flight * 2];
        render_si[2].signalSemaphoreCount = 1;
        render_si[2].pSignalSemaphores = &syncs[gs.current_frame].get_semaphore(Synchronization::S_LIGHTING_PASS_1_FINISHED);
        vmc.get_graphics_queue().submit(render_si, syncs[gs.current_frame].get_fence(Synchronization::F_RENDER_FINISHED));

        vk::PresentInfoKHR present_info{};
        present_info.sType = vk::StructureType::ePresentInfoKHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &syncs[gs.current_frame].get_semaphore(Synchronization::S_LIGHTING_PASS_1_FINISHED);
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain.get();
        present_info.pImageIndices = &image_idx;
        present_info.pResults = nullptr;
        VE_CHECK(vmc.get_present_queue().presentKHR(present_info), "Failed to present image!");
    }
} // namespace ve
