#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.hpp>

#include "vk/Buffer.hpp"
#include "vk/CommandPool.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanRenderContext.hpp"

namespace ve
{
    struct VulkanCommandContext {
        VulkanCommandContext(VulkanMainContext& vmc, VulkanRenderContext& vrc) : vmc(vmc), vrc(vrc), sync(vmc.logical_device.get())
        {
            const std::vector<ve::Vertex> vertices = {
                    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
                    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}};

            const std::vector<uint32_t> indices = {
                    0, 1, 2, 2, 3, 0};

            command_pools.push_back(CommandPool(vmc.logical_device.get(), vmc.queues_family_indices.graphics));
            command_pools.push_back(CommandPool(vmc.logical_device.get(), vmc.queues_family_indices.transfer));
            command_buffers.push_back(command_pools[0].create_command_buffers(vrc.frames_in_flight));
            command_buffers.push_back(command_pools[1].create_command_buffers(1));
            for (uint32_t i = 0; i < vrc.frames_in_flight; ++i)
            {
                sync_indices.push_back(sync.add_semaphore());
                sync_indices.push_back(sync.add_semaphore());
                sync_indices.push_back(sync.add_fence());
            }
            vrc.buffers.emplace(BufferNames::Vertex, Buffer(vmc, vertices, vk::BufferUsageFlagBits::eVertexBuffer, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, command_buffers[1][0]));
            vrc.buffers.emplace(BufferNames::Index, Buffer(vmc, indices, vk::BufferUsageFlagBits::eIndexBuffer, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)}, command_buffers[1][0]));
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Created VulkanCommandContext\n");
        }

        void self_destruct()
        {
            sync.wait_idle();
            for (auto& command_pool: command_pools)
            {
                command_pool.self_destruct();
            }
            command_pools.clear();
            sync.self_destruct();
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "Destroyed VulkanCommandContext\n");
        }

        VulkanMainContext& vmc;
        VulkanRenderContext& vrc;
        Synchronization sync;
        std::vector<CommandPool> command_pools;
        std::vector<std::vector<vk::CommandBuffer>> command_buffers;
        std::vector<uint32_t> sync_indices;

        void recreate_swapchain()
        {
            sync.wait_idle();
            vrc.recreate_swapchain();
        }

        void draw_frame()
        {
            glm::vec3 draw_sync_indices(sync_indices[0 + 3 * vrc.current_frame], sync_indices[1 + 3 * vrc.current_frame], sync_indices[2 + 3 * vrc.current_frame]);
            vk::ResultValue<uint32_t> image_idx = vmc.logical_device.get().acquireNextImageKHR(vrc.swapchain.get(), uint64_t(-1), sync.get_semaphore(draw_sync_indices.x));
            VE_CHECK(image_idx.result, "Failed to acquire next image!");
            sync.wait_for_fence(sync_indices[2 + 3 * vrc.current_frame]);
            sync.reset_fence(sync_indices[2 + 3 * vrc.current_frame]);
            reset_command_buffer(vrc.current_frame);
            record_graphics_command_buffer(vrc.current_frame, image_idx.value);
            submit_graphics(draw_sync_indices, vk::PipelineStageFlagBits::eColorAttachmentOutput, image_idx.value);
            vrc.current_frame = (vrc.current_frame + 1) % vrc.frames_in_flight;
        }

    private:
        void record_graphics_command_buffer(uint32_t idx, uint32_t image_idx)
        {
            VE_ASSERT(idx < command_buffers[0].size(), "Command buffer at requested index does not exist!\n");
            vk::CommandBufferBeginInfo cbbi{};
            cbbi.sType = vk::StructureType::eCommandBufferBeginInfo;
            cbbi.pInheritanceInfo = nullptr;
            command_buffers[0][idx].begin(cbbi);
            vk::RenderPassBeginInfo rpbi{};
            rpbi.sType = vk::StructureType::eRenderPassBeginInfo;
            rpbi.renderPass = vrc.render_pass.get();
            rpbi.framebuffer = vrc.swapchain.get_framebuffer(image_idx);
            rpbi.renderArea.offset = vk::Offset2D(0, 0);
            rpbi.renderArea.extent = vrc.swapchain.get_extent();
            vk::ClearValue clear_color;
            clear_color.color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
            rpbi.clearValueCount = 1;
            rpbi.pClearValues = &clear_color;
            command_buffers[0][idx].beginRenderPass(rpbi, vk::SubpassContents::eInline);
            command_buffers[0][idx].bindPipeline(vk::PipelineBindPoint::eGraphics, vrc.pipeline.get());

            vk::Viewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = vrc.swapchain.get_extent().width;
            viewport.height = vrc.swapchain.get_extent().height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            command_buffers[0][idx].setViewport(0, viewport);
            vk::Rect2D scissor{};
            scissor.offset = vk::Offset2D(0, 0);
            scissor.extent = vrc.swapchain.get_extent();
            command_buffers[0][idx].setScissor(0, scissor);

            command_buffers[0][idx].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vrc.pipeline.get_layout(), 0, vrc.descriptor_set_handler.get_sets()[vrc.current_frame], {});

            std::vector<vk::DeviceSize> offsets(1, 0);
            command_buffers[0][idx].bindVertexBuffers(0, vrc.buffers.find(BufferNames::Vertex)->second.get(), offsets);
            command_buffers[0][idx].bindIndexBuffer(vrc.buffers.find(BufferNames::Index)->second.get(), 0, vk::IndexType::eUint32);
            command_buffers[0][idx].drawIndexed(vrc.buffers.find(BufferNames::Index)->second.get_element_count(), 1, 0, 0, 0);
            command_buffers[0][idx].endRenderPass();
            command_buffers[0][idx].end();
        }

        void reset_command_buffer(uint32_t idx)
        {
            command_buffers[0][idx].reset();
        }

        void submit_graphics(glm::vec3 sync_indices, vk::PipelineStageFlags wait_stage, uint32_t image_idx)
        {
            vk::SubmitInfo si{};
            si.sType = vk::StructureType::eSubmitInfo;
            si.waitSemaphoreCount = 1;
            si.pWaitSemaphores = &sync.get_semaphore(sync_indices.x);
            si.pWaitDstStageMask = &wait_stage;
            si.commandBufferCount = 1;
            si.pCommandBuffers = &command_buffers[0][vrc.current_frame];
            si.signalSemaphoreCount = 1;
            si.pSignalSemaphores = &sync.get_semaphore(sync_indices.y);
            vmc.get_graphics_queue().submit(si, sync.get_fence(sync_indices.z));

            vk::PresentInfoKHR present_info{};
            present_info.sType = vk::StructureType::ePresentInfoKHR;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &sync.get_semaphore(sync_indices.y);
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &vrc.swapchain.get();
            present_info.pImageIndices = &image_idx;
            present_info.pResults = nullptr;
            VE_CHECK(vmc.get_present_queue().presentKHR(present_info), "Failed to present image!");
        }
    };
}// namespace ve
