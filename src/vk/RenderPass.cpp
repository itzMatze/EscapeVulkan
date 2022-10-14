#include "vk/RenderPass.hpp"

namespace ve
{
    RenderPass::RenderPass(const VulkanMainContext& vmc, const vk::Format& format) : vmc(vmc)
    {
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

        vk::SubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        vk::RenderPassCreateInfo rpci{};
        rpci.sType = vk::StructureType::eRenderPassCreateInfo;
        rpci.attachmentCount = 1;
        rpci.pAttachments = &ad;
        rpci.subpassCount = 1;
        rpci.pSubpasses = &spd;
        rpci.dependencyCount = 1;
        rpci.pDependencies = &dependency;

        render_pass = vmc.logical_device.get().createRenderPass(rpci);
    }

    const vk::RenderPass RenderPass::get() const
    {
        return render_pass;
    }

    void RenderPass::self_destruct()
    {
        vmc.logical_device.get().destroyRenderPass(render_pass);
    }
}// namespace ve
