#include "vk/RenderPass.hpp"

namespace ve
{
    RenderPass::RenderPass(const VulkanMainContext& vmc, const vk::Format& color_format, const vk::Format& depth_format) : vmc(vmc)
    {
        vk::AttachmentDescription color_ad{};
        color_ad.format = color_format;
        color_ad.samples = vk::SampleCountFlagBits::e1;
        color_ad.loadOp = vk::AttachmentLoadOp::eClear;
        color_ad.storeOp = vk::AttachmentStoreOp::eStore;
        color_ad.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        color_ad.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        color_ad.initialLayout = vk::ImageLayout::eUndefined;
        color_ad.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentDescription depth_ad{};
        depth_ad.format = depth_format;
        depth_ad.samples = vk::SampleCountFlagBits::e1;
        depth_ad.loadOp = vk::AttachmentLoadOp::eClear;
        depth_ad.storeOp = vk::AttachmentStoreOp::eDontCare;
        depth_ad.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depth_ad.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depth_ad.initialLayout = vk::ImageLayout::eUndefined;
        depth_ad.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference color_ar{};
        // index of AttachmentDescription in Array, here we only have one, so index = 0
        color_ar.attachment = 0;
        color_ar.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference depth_ar{};
        depth_ar.attachment = 1;
        depth_ar.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::SubpassDescription spd{};
        spd.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        spd.colorAttachmentCount = 1;
        spd.pColorAttachments = &color_ar;
        spd.pDepthStencilAttachment = &depth_ar;

        vk::SubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        std::array<vk::AttachmentDescription, 2> attachments = {color_ad, depth_ad};
        vk::RenderPassCreateInfo rpci{};
        rpci.sType = vk::StructureType::eRenderPassCreateInfo;
        rpci.attachmentCount = attachments.size();
        rpci.pAttachments = attachments.data();
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
