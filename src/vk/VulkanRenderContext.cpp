#include "vk/VulkanRenderContext.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "vk/common.hpp"

namespace ve
{
    VulkanRenderContext::VulkanRenderContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc) : vmc(vmc), vcc(vcc), descriptor_set_handler(vmc), surface_format(choose_surface_format()), depth_format(choose_depth_format()), render_pass(vmc, surface_format.format, depth_format), swapchain(vmc, surface_format, depth_format, render_pass.get()), pipeline(vmc)
    {
        vcc.add_graphics_buffers(frames_in_flight);
        vcc.add_transfer_buffers(1);

        const std::vector<ve::Vertex> vertices = {
                {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

                {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

        const std::vector<uint32_t> indices = {
                0, 1, 2, 2, 3, 0,
                4, 5, 6, 6, 7, 4};

        images.emplace(ImageNames::Texture, Image(vmc, vcc, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, "white.png"));

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            uniform_buffers.push_back(Buffer(vmc, std::vector<UniformBufferObject>{ubo}, vk::BufferUsageFlagBits::eUniformBuffer, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}));
        }

        descriptor_set_handler.add_uniform_buffer(frames_in_flight, uniform_buffers);
        descriptor_set_handler.add_image_binding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, images.find(ImageNames::Texture)->second);
        descriptor_set_handler.construct();
        pipeline.construct(render_pass.get(), descriptor_set_handler);

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            sync_indices[SyncNames::SImageAvailable].push_back(vcc.sync.add_semaphore());
            sync_indices[SyncNames::SRenderFinished].push_back(vcc.sync.add_semaphore());
            sync_indices[SyncNames::FRenderFinished].push_back(vcc.sync.add_fence());
        }

        buffers.emplace(BufferNames::Vertex, Buffer(vmc, vertices, vk::BufferUsageFlagBits::eVertexBuffer, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, vcc));
        buffers.emplace(BufferNames::Index, Buffer(vmc, indices, vk::BufferUsageFlagBits::eIndexBuffer, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, vcc));
        VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Created VulkanRenderContext\n");
    }

    void VulkanRenderContext::self_destruct()
    {
        for (auto& image: images)
        {
            image.second.self_destruct();
        }
        for (auto& buffer: buffers)
        {
            buffer.second.self_destruct();
        }
        buffers.clear();
        for (auto& buffer: uniform_buffers)
        {
            buffer.self_destruct();
        }
        uniform_buffers.clear();
        pipeline.self_destruct();
        swapchain.self_destruct();
        render_pass.self_destruct();
        descriptor_set_handler.self_destruct();
        VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Destroyed VulkanRenderContext\n");
    }

    void VulkanRenderContext::draw_frame()
    {
        vk::ResultValue<uint32_t> image_idx = vmc.logical_device.get().acquireNextImageKHR(swapchain.get(), uint64_t(-1), vcc.sync.get_semaphore(sync_indices[SyncNames::SImageAvailable][current_frame]));
        VE_CHECK(image_idx.result, "Failed to acquire next image!");
        vcc.sync.wait_for_fence(sync_indices[SyncNames::FRenderFinished][current_frame]);
        vcc.sync.reset_fence(sync_indices[SyncNames::FRenderFinished][current_frame]);
        record_graphics_command_buffer(image_idx.value);
        submit_graphics(vk::PipelineStageFlagBits::eColorAttachmentOutput, image_idx.value);
        current_frame = (current_frame + 1) % frames_in_flight;
    }

    void VulkanRenderContext::recreate_swapchain()
    {
        vcc.sync.wait_idle();
        swapchain.self_destruct();
        swapchain.create_swapchain(surface_format, depth_format, render_pass.get());
    }

    void VulkanRenderContext::update_uniform_data(float time_diff, const glm::mat4& vp)
    {
        ubo.M = glm::rotate(ubo.M, time_diff * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
        pc.MVP = vp * ubo.M;
        uniform_buffers[current_frame].update_data(ubo);
    }

    vk::SurfaceFormatKHR VulkanRenderContext::choose_surface_format()
    {
        std::vector<vk::SurfaceFormatKHR> formats = vmc.get_surface_formats();
        for (const auto& format: formats)
        {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) return format;
        }
        VE_LOG_CONSOLE(VE_WARN, VE_C_YELLOW << "Desired format not found. Using first available.");
        return formats[0];
    }

    vk::Format VulkanRenderContext::choose_depth_format()
    {
        std::vector<vk::Format> candidates{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}; 
        for (vk::Format format: candidates)
        {
            vk::FormatProperties props = vmc.physical_device.get().getFormatProperties(format);
            if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            {
                return format;
            }
        }
        VE_THROW("Failed to find supported format!");
    }

    void VulkanRenderContext::record_graphics_command_buffer(uint32_t image_idx)
    {
        vcc.begin(vcc.graphics_cb[current_frame]);
        vk::RenderPassBeginInfo rpbi{};
        rpbi.sType = vk::StructureType::eRenderPassBeginInfo;
        rpbi.renderPass = render_pass.get();
        rpbi.framebuffer = swapchain.get_framebuffer(image_idx);
        rpbi.renderArea.offset = vk::Offset2D(0, 0);
        rpbi.renderArea.extent = swapchain.get_extent();
        std::array<vk::ClearValue, 2> clear_values{};
        clear_values[0].color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
        clear_values[1].depthStencil.depth = 1.0f;
        clear_values[1].depthStencil.stencil = 0;
        rpbi.clearValueCount = clear_values.size();
        rpbi.pClearValues = clear_values.data();
        vcc.graphics_cb[current_frame].beginRenderPass(rpbi, vk::SubpassContents::eInline);
        vcc.graphics_cb[current_frame].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());

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

        vcc.graphics_cb[current_frame].pushConstants(pipeline.get_layout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &pc);
        vcc.graphics_cb[current_frame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.get_layout(), 0, descriptor_set_handler.get_sets()[current_frame], {});

        std::vector<vk::DeviceSize> offsets(1, 0);
        vcc.graphics_cb[current_frame].bindVertexBuffers(0, buffers.find(BufferNames::Vertex)->second.get(), offsets);
        vcc.graphics_cb[current_frame].bindIndexBuffer(buffers.find(BufferNames::Index)->second.get(), 0, vk::IndexType::eUint32);
        vcc.graphics_cb[current_frame].drawIndexed(buffers.find(BufferNames::Index)->second.get_element_count(), 1, 0, 0, 0);
        vcc.graphics_cb[current_frame].endRenderPass();
        vcc.graphics_cb[current_frame].end();
    }

    void VulkanRenderContext::submit_graphics(vk::PipelineStageFlags wait_stage, uint32_t image_idx)
    {
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
}// namespace ve
