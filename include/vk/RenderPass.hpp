#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"

namespace ve
{
    class RenderPass
    {
    public:
        RenderPass() = default;
        RenderPass(const vk::Device device, const vk::Format format)
        {
            VE_LOG_CONSOLE(PINK << "Creating render pass");
            vk::AttachmentDescription ad{};
            ad.format = format;
            ad.samples = vk::SampleCountFlagBits::e1;
            ad.loadOp = vk::AttachmentLoadOp::eClear;
            ad.storeOp = vk::AttachmentStoreOp::eStore;
            ad.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            ad.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            ad.initialLayout = vk::ImageLayout::eUndefined;
            ad.finalLayout = vk::ImageLayout::ePresentSrcKHR;

            vk::AttachmentReference ar{};
            // index of AttachmentDescription in Array, here we only have one, so index = 0
            ar.attachment = 0;
            ar.layout = vk::ImageLayout::eColorAttachmentOptimal;

            vk::SubpassDescription spd{};
            spd.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            spd.colorAttachmentCount = 1;
            spd.pColorAttachments = &ar;

            vk::RenderPassCreateInfo rpci{};
            rpci.sType = vk::StructureType::eRenderPassCreateInfo;
            rpci.attachmentCount = 1;
            rpci.pAttachments = &ad;
            rpci.subpassCount = 1;
            rpci.pSubpasses = &spd;

            render_pass = device.createRenderPass(rpci);
        }

        const vk::RenderPass get() const
        {
            return render_pass;
        }

        void self_destruct(const vk::Device device)
        {
            VE_LOG_CONSOLE(PINK << "Destroying render pass");
            device.destroyRenderPass(render_pass);
        }

    private:
        vk::RenderPass render_pass;
    };
}// namespace ve
