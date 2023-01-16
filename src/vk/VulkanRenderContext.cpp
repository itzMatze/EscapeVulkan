#include "vk/VulkanRenderContext.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "vk/common.hpp"

namespace ve
{
    VulkanRenderContext::VulkanRenderContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc) : vmc(vmc), vcc(vcc), swapchain(vmc, choose_sample_count()), scene(vmc, vcc)
    {
        vcc.add_graphics_buffers(frames_in_flight);
        vcc.add_transfer_buffers(1);

        scene.load("../assets/scenes/default.json");

        // add one descriptor set for every frame
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            uniform_buffers.push_back(Buffer(vmc, std::vector<UniformBufferObject>{ubo}, vk::BufferUsageFlagBits::eUniformBuffer, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}));
            scene.get_dsh(ShaderFlavor::Default).apply_descriptor_to_new_sets(0, uniform_buffers.back());
            scene.get_dsh(ShaderFlavor::Basic).apply_descriptor_to_new_sets(0, uniform_buffers.back());
            scene.add_bindings();
            scene.get_dsh(ShaderFlavor::Default).reset_auto_apply_bindings();
            scene.get_dsh(ShaderFlavor::Basic).reset_auto_apply_bindings();
        }

        scene.construct(swapchain.get_render_pass());

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            sync_indices[SyncNames::SImageAvailable].push_back(vcc.sync.add_semaphore());
            sync_indices[SyncNames::SRenderFinished].push_back(vcc.sync.add_semaphore());
            sync_indices[SyncNames::FRenderFinished].push_back(vcc.sync.add_fence());
        }

        spdlog::info("Created VulkanRenderContext");
    }

    void VulkanRenderContext::self_destruct()
    {
        for (auto& buffer: uniform_buffers)
        {
            buffer.self_destruct();
        }
        scene.self_destruct();
        uniform_buffers.clear();
        swapchain.self_destruct(true);
        spdlog::info("Destroyed VulkanRenderContext");
    }

    void VulkanRenderContext::draw_frame(const Camera& camera, float time_diff)
    {
        total_time += time_diff;
        ubo.M = glm::rotate(ubo.M, time_diff * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
        scene.rotate("bunny", time_diff * 90.f, glm::vec3(0.0f, 1.0f, 0.0f));
        pc.MVP = camera.getVP() * ubo.M;
        uniform_buffers[current_frame].update_data(ubo);

        vk::ResultValue<uint32_t> image_idx = vmc.logical_device.get().acquireNextImageKHR(swapchain.get(), uint64_t(-1), vcc.sync.get_semaphore(sync_indices[SyncNames::SImageAvailable][current_frame]));
        VE_CHECK(image_idx.result, "Failed to acquire next image!");
        vcc.sync.wait_for_fence(sync_indices[SyncNames::FRenderFinished][current_frame]);
        vcc.sync.reset_fence(sync_indices[SyncNames::FRenderFinished][current_frame]);
        record_graphics_command_buffer(image_idx.value, camera.getVP());
        submit_graphics(image_idx.value);
        current_frame = (current_frame + 1) % frames_in_flight;
    }

    vk::Extent2D VulkanRenderContext::recreate_swapchain()
    {
        vcc.sync.wait_idle();
        swapchain.self_destruct(false);
        swapchain.create_swapchain();
        return swapchain.get_extent();
    }

    void VulkanRenderContext::record_graphics_command_buffer(uint32_t image_idx, const glm::mat4& vp)
    {
        vcc.begin(vcc.graphics_cb[current_frame]);
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
        vcc.graphics_cb[current_frame].beginRenderPass(rpbi, vk::SubpassContents::eInline);

        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = swapchain.get_extent().width;
        viewport.height = swapchain.get_extent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vcc.graphics_cb[current_frame].setViewport(0, viewport);
        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D(0, 0);
        scissor.extent = swapchain.get_extent();
        vcc.graphics_cb[current_frame].setScissor(0, scissor);

        std::vector<vk::DeviceSize> offsets(1, 0);

        scene.draw(vcc.graphics_cb[current_frame], current_frame, vp);

        vcc.graphics_cb[current_frame].endRenderPass();
        vcc.graphics_cb[current_frame].end();
    }

    void VulkanRenderContext::submit_graphics(uint32_t image_idx)
    {
        vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo si{};
        si.sType = vk::StructureType::eSubmitInfo;
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = &vcc.sync.get_semaphore(sync_indices[SyncNames::SImageAvailable][current_frame]);
        si.pWaitDstStageMask = &wait_stage;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &vcc.graphics_cb[current_frame];
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = &vcc.sync.get_semaphore(sync_indices[SyncNames::SRenderFinished][current_frame]);
        vmc.get_graphics_queue().submit(si, vcc.sync.get_fence(sync_indices[SyncNames::FRenderFinished][current_frame]));

        vk::PresentInfoKHR present_info{};
        present_info.sType = vk::StructureType::ePresentInfoKHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &vcc.sync.get_semaphore(sync_indices[SyncNames::SRenderFinished][current_frame]);
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain.get();
        present_info.pImageIndices = &image_idx;
        present_info.pResults = nullptr;
        VE_CHECK(vmc.get_present_queue().presentKHR(present_info), "Failed to present image!");
    }

    vk::SampleCountFlagBits VulkanRenderContext::choose_sample_count()
    {
        vk::PhysicalDeviceProperties pdp = vmc.physical_device.get().getProperties();
        vk::Flags<vk::SampleCountFlagBits> counts = (pdp.limits.framebufferColorSampleCounts & pdp.limits.framebufferDepthSampleCounts);
        if (counts & vk::SampleCountFlagBits::e4) return vk::SampleCountFlagBits::e4;
        if (counts & vk::SampleCountFlagBits::e2) return vk::SampleCountFlagBits::e2;
        if (counts & vk::SampleCountFlagBits::e8) return vk::SampleCountFlagBits::e8;
        if (counts & vk::SampleCountFlagBits::e16) return vk::SampleCountFlagBits::e16;
        if (counts & vk::SampleCountFlagBits::e32) return vk::SampleCountFlagBits::e32;
        if (counts & vk::SampleCountFlagBits::e64) return vk::SampleCountFlagBits::e64;
        return vk::SampleCountFlagBits::e1;
    }
}// namespace ve
