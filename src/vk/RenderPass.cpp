#include "vk/RenderPass.hpp"

namespace ve
{
    RenderPass::RenderPass(const VulkanMainContext& vmc, const vk::Format& color_format, const vk::Format& depth_format) : vmc(vmc), sample_count(choose_sample_count())
    {
        vk::AttachmentDescription color_ad{};
        color_ad.format = color_format;
        color_ad.samples = sample_count;
        color_ad.loadOp = vk::AttachmentLoadOp::eClear;
        color_ad.storeOp = vk::AttachmentStoreOp::eStore;
        color_ad.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        color_ad.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        color_ad.initialLayout = vk::ImageLayout::eUndefined;
        color_ad.finalLayout = sample_count != vk::SampleCountFlagBits::e1 ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentDescription depth_ad{};
        depth_ad.format = depth_format;
        depth_ad.samples = sample_count;
        depth_ad.loadOp = vk::AttachmentLoadOp::eClear;
        depth_ad.storeOp = vk::AttachmentStoreOp::eDontCare;
        depth_ad.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depth_ad.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depth_ad.initialLayout = vk::ImageLayout::eUndefined;
        depth_ad.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentDescription color_ad_resolve{};
        color_ad_resolve.format = color_format;
        color_ad_resolve.samples = vk::SampleCountFlagBits::e1;
        color_ad_resolve.loadOp = vk::AttachmentLoadOp::eDontCare;
        color_ad_resolve.storeOp = vk::AttachmentStoreOp::eStore;
        color_ad_resolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        color_ad_resolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        color_ad_resolve.initialLayout = vk::ImageLayout::eUndefined;
        color_ad_resolve.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference color_ar{};
        color_ar.attachment = 0;
        color_ar.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference depth_ar{};
        depth_ar.attachment = 1;
        depth_ar.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference color_ar_resolve{};
        color_ar_resolve.attachment = 2;
        color_ar_resolve.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription spd{};
        spd.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        spd.colorAttachmentCount = 1;
        spd.pColorAttachments = &color_ar;
        spd.pDepthStencilAttachment = &depth_ar;
        if (sample_count != vk::SampleCountFlagBits::e1) spd.pResolveAttachments = &color_ar_resolve;

        vk::SubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        std::vector<vk::AttachmentDescription> attachments{color_ad, depth_ad};
        if (sample_count != vk::SampleCountFlagBits::e1) attachments.push_back(color_ad_resolve);
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

    vk::RenderPass RenderPass::get() const
    {
        return render_pass;
    }

    vk::SampleCountFlagBits RenderPass::get_sample_count() const
    {
        return sample_count;
    }

    void RenderPass::self_destruct()
    {
        vmc.logical_device.get().destroyRenderPass(render_pass);
    }

    vk::SampleCountFlagBits RenderPass::choose_sample_count()
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
} // namespace ve
