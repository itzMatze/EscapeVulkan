#pragma once

#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "vk/Buffer.hpp"
#include "vk/DescriptorSetHandler.hpp"
#include "vk/Pipeline.hpp"
#include "vk/RenderPass.hpp"
#include "vk/Swapchain.hpp"

namespace ve
{
    struct UniformBufferObject {
        glm::mat4 M;
        glm::mat4 VP;
    };

    class VulkanRenderContext
    {
    public:
        VulkanRenderContext(const VulkanMainContext& vmc, VulkanCommandContext& vcc) : vmc(vmc), vcc(vcc), descriptor_set_handler(vmc), surface_format(choose_surface_format(vmc)), render_pass(vmc, surface_format.format), swapchain(vmc, surface_format, render_pass.get()), pipeline(vmc)
        {
            const std::vector<ve::Vertex> vertices = {
                    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
                    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}};

            const std::vector<uint32_t> indices = {
                    0, 1, 2, 2, 3, 0};

            uniform_buffers = descriptor_set_handler.add_uniform_buffer(frames_in_flight, std::vector<UniformBufferObject>{ubo}, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)});
            descriptor_set_handler.construct();
            pipeline.construct(render_pass.get(), descriptor_set_handler);

            vcc.add_graphics_buffers(frames_in_flight);
            vcc.add_transfer_buffers(1);
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

        void self_destruct()
        {
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

    private:
        enum class BufferNames
        {
            Vertex,
            Index
        };

        enum class SyncNames
        {
            SImageAvailable,
            SRenderFinished,
            FRenderFinished
        };

    public:
        UniformBufferObject ubo{
                glm::mat4(1.0f),
                glm::mat4(1.0f)};

        const uint32_t frames_in_flight = 2;
        uint32_t current_frame = 0;
        const VulkanMainContext& vmc;
        VulkanCommandContext& vcc;
        DescriptorSetHandler descriptor_set_handler;
        std::vector<ve::Buffer> uniform_buffers;
        std::unordered_map<SyncNames, std::vector<uint32_t>> sync_indices;
        std::unordered_map<BufferNames, Buffer> buffers;
        vk::SurfaceFormatKHR surface_format;
        RenderPass render_pass;
        Swapchain swapchain;
        Pipeline pipeline;

        void draw_frame()
        {
            vk::ResultValue<uint32_t> image_idx = vmc.logical_device.get().acquireNextImageKHR(swapchain.get(), uint64_t(-1), vcc.sync.get_semaphore(sync_indices[SyncNames::SImageAvailable][current_frame]));
            VE_CHECK(image_idx.result, "Failed to acquire next image!");
            vcc.sync.wait_for_fence(sync_indices[SyncNames::FRenderFinished][current_frame]);
            vcc.sync.reset_fence(sync_indices[SyncNames::FRenderFinished][current_frame]);
            record_graphics_command_buffer(image_idx.value);
            submit_graphics(vk::PipelineStageFlagBits::eColorAttachmentOutput, image_idx.value);
            current_frame = (current_frame + 1) % frames_in_flight;
        }

        void recreate_swapchain()
        {
            vcc.sync.wait_idle();
            swapchain.self_destruct();
            swapchain.create_swapchain(surface_format, render_pass.get());
        }

        void update_uniform_data(float time_diff, const glm::mat4& vp)
        {
            ubo.M = glm::rotate(ubo.M, time_diff * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.VP = vp;
            uniform_buffers[current_frame].update_data(ubo);
        }

    private:
        vk::SurfaceFormatKHR choose_surface_format(const VulkanMainContext& vmc)
        {
            std::vector<vk::SurfaceFormatKHR> formats = vmc.get_surface_formats();
            for (const auto& format: formats)
            {
                if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) return format;
            }
            VE_LOG_CONSOLE(VE_WARN, VE_C_YELLOW << "Desired format not found. Using first available.");
            return formats[0];
        }

        void record_graphics_command_buffer(uint32_t image_idx)
        {
            vcc.begin(vcc.graphics_cb[current_frame]);
            vk::RenderPassBeginInfo rpbi{};
            rpbi.sType = vk::StructureType::eRenderPassBeginInfo;
            rpbi.renderPass = render_pass.get();
            rpbi.framebuffer = swapchain.get_framebuffer(image_idx);
            rpbi.renderArea.offset = vk::Offset2D(0, 0);
            rpbi.renderArea.extent = swapchain.get_extent();
            vk::ClearValue clear_color;
            clear_color.color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
            rpbi.clearValueCount = 1;
            rpbi.pClearValues = &clear_color;
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

            vcc.graphics_cb[current_frame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.get_layout(), 0, descriptor_set_handler.get_sets()[current_frame], {});

            std::vector<vk::DeviceSize> offsets(1, 0);
            vcc.graphics_cb[current_frame].bindVertexBuffers(0, buffers.find(BufferNames::Vertex)->second.get(), offsets);
            vcc.graphics_cb[current_frame].bindIndexBuffer(buffers.find(BufferNames::Index)->second.get(), 0, vk::IndexType::eUint32);
            vcc.graphics_cb[current_frame].drawIndexed(buffers.find(BufferNames::Index)->second.get_element_count(), 1, 0, 0, 0);
            vcc.graphics_cb[current_frame].endRenderPass();
            vcc.graphics_cb[current_frame].end();
        }

        void submit_graphics(vk::PipelineStageFlags wait_stage, uint32_t image_idx)
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
    };
}// namespace ve