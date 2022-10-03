#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"

namespace ve
{
    class CommandPool
    {
    public:
        CommandPool(const vk::Device logical_device, uint32_t queue_family_idx) : device(logical_device)
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "command pool +\n");
            vk::CommandPoolCreateInfo cpci{};
            cpci.sType = vk::StructureType::eCommandPoolCreateInfo;
            cpci.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            cpci.queueFamilyIndex = queue_family_idx;
            VE_LOG_CONSOLE(VE_INFO, "Creating command pool\n");
            command_pool = device.createCommandPool(cpci);
                        
            vk::CommandBufferAllocateInfo cbai{};
            cbai.sType = vk::StructureType::eCommandBufferAllocateInfo;
            cbai.commandPool = command_pool;
            // secondary command buffers can be called from primary command buffers
            cbai.level = vk::CommandBufferLevel::ePrimary;
            cbai.commandBufferCount = 1;
            VE_LOG_CONSOLE(VE_INFO, "Creating command buffer\n");
            command_buffers = device.allocateCommandBuffers(cbai);
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "command pool +++\n");
        }

        void record_command_buffer(uint32_t command_buffer_idx, const Swapchain& swapchain, const vk::RenderPass render_pass, const vk::Pipeline pipeline)
        {
            VE_ASSERT(command_buffer_idx < command_buffers.size(), "Command buffer at requested index does not exist!\n");
            vk::CommandBufferBeginInfo cbbi{};
            cbbi.sType = vk::StructureType::eCommandBufferBeginInfo;
            cbbi.pInheritanceInfo = nullptr;
            command_buffers[command_buffer_idx].begin(cbbi);
            vk::RenderPassBeginInfo rpbi{};
            rpbi.sType = vk::StructureType::eRenderPassBeginInfo;
            rpbi.renderPass = render_pass;
            rpbi.framebuffer = swapchain.get_framebuffer();
            rpbi.renderArea.offset = vk::Offset2D(0, 0);
            rpbi.renderArea.extent = swapchain.get_extent();
            vk::ClearValue clear_color;
            clear_color.color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
            rpbi.clearValueCount = 1;
            rpbi.pClearValues = &clear_color;
            command_buffers[command_buffer_idx].beginRenderPass(rpbi, vk::SubpassContents::eInline);
            command_buffers[command_buffer_idx].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

            vk::Viewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = swapchain.get_extent().width;
            viewport.height = swapchain.get_extent().height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            command_buffers[command_buffer_idx].setViewport(0, viewport);
            vk::Rect2D scissor{};
            scissor.offset = vk::Offset2D(0, 0);
            scissor.extent = swapchain.get_extent();
            command_buffers[command_buffer_idx].setScissor(0, scissor);
            command_buffers[command_buffer_idx].draw(3, 1, 0, 0);
            command_buffers[command_buffer_idx].endRenderPass();
            command_buffers[command_buffer_idx].end();
        }

        ~CommandPool()
        {
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "command pool -\n");
            VE_LOG_CONSOLE(VE_INFO, "Destroying command pool\n");
            device.destroyCommandPool(command_pool);
            VE_LOG_CONSOLE(VE_INFO, VE_C_PINK << "command pool ---\n");
        }

    private:
        const vk::Device device;
        vk::CommandPool command_pool;
        std::vector<vk::CommandBuffer> command_buffers;
    };
}// namespace ve