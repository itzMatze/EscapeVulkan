#include "vk/VulkanRenderContext.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "vk/common.hpp"

namespace ve
{
    VulkanRenderContext::VulkanRenderContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc) : vmc(vmc), vcc(vcc), swapchain(vmc)
    {
        vcc.add_graphics_buffers(frames_in_flight);
        vcc.add_transfer_buffers(1);

        const std::vector<ve::Vertex> vertices_one = {
                {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
                {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
                {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

        const std::vector<ve::Vertex> vertices_two = {
                {{-500.0f, -500.0f, -500.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                {{500.0f, -500.0f, -500.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
                {{500.0f, -500.0f, 500.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
                {{-500.0f, -500.0f, 500.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

        const std::vector<uint32_t> indices_one = {
                0, 1, 2, 2, 3, 0};

        const std::vector<uint32_t> indices_two = {
                0, 1, 2, 2, 3, 0};

        images.emplace_back(Image(vmc, vcc, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, "../assets/textures/white.png"));
        Material m1{};
        m1.base_texture = &(images[0]);
        ros.emplace(ShaderFlavor::Default, vmc);
        ros.emplace(ShaderFlavor::Basic, vmc);

        scene_handles.emplace("floor", SceneHandle(ShaderFlavor::Basic, &vertices_two, &indices_two, nullptr));

        for (auto& scene_handle: scene_handles)
        {
            if (scene_handle.second.filename != "none")
            {
                scene_handle.second.idx = ros.at(scene_handle.second.shader_flavor).add_scene(vcc, scene_handle.second.filename);
            }
            else
            {
                scene_handle.second.idx = ros.at(scene_handle.second.shader_flavor).add_scene(vcc, *scene_handle.second.vertices, *scene_handle.second.indices, scene_handle.second.material);
            }
        }
        ros.at(ShaderFlavor::Default).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
        ros.at(ShaderFlavor::Default).dsh.add_binding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);

        ros.at(ShaderFlavor::Basic).dsh.add_binding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            uniform_buffers.push_back(Buffer(vmc, std::vector<UniformBufferObject>{ubo}, vk::BufferUsageFlagBits::eUniformBuffer, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}));
            ros.at(ShaderFlavor::Default).dsh.apply_descriptor_to_new_sets(0, uniform_buffers.back());
            ros.at(ShaderFlavor::Basic).dsh.apply_descriptor_to_new_sets(0, uniform_buffers.back());
            for (auto& ro: ros)
            {
                ro.second.add_bindings();
            }
            ros.at(ShaderFlavor::Default).dsh.reset_auto_apply_bindings();
            ros.at(ShaderFlavor::Basic).dsh.reset_auto_apply_bindings();
        }

        ros.at(ShaderFlavor::Default).construct(swapchain.get_render_pass(), {std::make_pair("default.vert", vk::ShaderStageFlagBits::eVertex), std::make_pair("default.frag", vk::ShaderStageFlagBits::eFragment)}, vk::PolygonMode::eFill);
        ros.at(ShaderFlavor::Basic).construct(swapchain.get_render_pass(), {std::make_pair("default.vert", vk::ShaderStageFlagBits::eVertex), std::make_pair("basic.frag", vk::ShaderStageFlagBits::eFragment)}, vk::PolygonMode::eFill);

        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            sync_indices[SyncNames::SImageAvailable].push_back(vcc.sync.add_semaphore());
            sync_indices[SyncNames::SRenderFinished].push_back(vcc.sync.add_semaphore());
            sync_indices[SyncNames::FRenderFinished].push_back(vcc.sync.add_fence());
        }

        VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Created VulkanRenderContext\n");
    }

    void VulkanRenderContext::self_destruct()
    {
        for (auto& image: images)
        {
            image.self_destruct();
        }
        images.clear();
        for (auto& buffer: uniform_buffers)
        {
            buffer.self_destruct();
        }
        for (auto& ro: ros)
        {
            ro.second.self_destruct();
        }
        ros.clear();
        uniform_buffers.clear();
        swapchain.self_destruct(true);
        VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Destroyed VulkanRenderContext\n");
    }

    void VulkanRenderContext::draw_frame(const Camera& camera, float time_diff)
    {
        ubo.M = glm::rotate(ubo.M, time_diff * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
        get_scene("floor")->rotate(time_diff * 90.f, glm::vec3(0.0f, 0.0f, 1.0f));
        pc.MVP = camera.getVP() * ubo.M;
        uniform_buffers[current_frame].update_data(ubo);

        vk::ResultValue<uint32_t> image_idx = vmc.logical_device.get().acquireNextImageKHR(swapchain.get(), uint64_t(-1), vcc.sync.get_semaphore(sync_indices[SyncNames::SImageAvailable][current_frame]));
        VE_CHECK(image_idx.result, "Failed to acquire next image!");
        vcc.sync.wait_for_fence(sync_indices[SyncNames::FRenderFinished][current_frame]);
        vcc.sync.reset_fence(sync_indices[SyncNames::FRenderFinished][current_frame]);
        record_graphics_command_buffer(image_idx.value, camera.getVP());
        submit_graphics(vk::PipelineStageFlagBits::eColorAttachmentOutput, image_idx.value);
        current_frame = (current_frame + 1) % frames_in_flight;
    }

    vk::Extent2D VulkanRenderContext::recreate_swapchain()
    {
        vcc.sync.wait_idle();
        swapchain.self_destruct(false);
        swapchain.create_swapchain();
        return swapchain.get_extent();
    }

    Scene* VulkanRenderContext::get_scene(const std::string& key)
    {
        if (scene_handles.contains(key))
        {
            return ros.at(scene_handles.at(key).shader_flavor).get_scene(scene_handles.at(key).idx);
        }
        return nullptr;
    }

    void VulkanRenderContext::record_graphics_command_buffer(uint32_t image_idx, const glm::mat4& vp)
    {
        vcc.begin(vcc.graphics_cb[current_frame]);
        vk::RenderPassBeginInfo rpbi{};
        rpbi.sType = vk::StructureType::eRenderPassBeginInfo;
        rpbi.renderPass = swapchain.get_render_pass();
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

        for (auto& ro: ros)
        {
            ro.second.draw(vcc.graphics_cb[current_frame], current_frame, vp);
        }

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
