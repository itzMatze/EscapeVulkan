#include "vk/RenderPass.hpp"

namespace ve
{
    RenderPass::RenderPass(const VulkanMainContext& vmc, const vk::Format& color_format, const vk::Format& depth_format) : vmc(vmc)
    {
        attachment_count = 1;
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

        std::vector<vk::AttachmentDescription> attachments{color_ad, depth_ad};
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

    RenderPass::RenderPass(const VulkanMainContext& vmc, const vk::Format& depth_format) : vmc(vmc)
    {
        attachment_count = 4;
        // deferred rendering render pass
		std::vector<vk::AttachmentDescription> attachments(5);
		for (uint32_t i = 0; i < attachments.size(); ++i)
		{
            attachments[i].samples = vk::SampleCountFlagBits::e1;
            attachments[i].loadOp = vk::AttachmentLoadOp::eClear;
            attachments[i].storeOp = vk::AttachmentStoreOp::eStore;
            attachments[i].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            attachments[i].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            attachments[i].initialLayout = vk::ImageLayout::eUndefined;
            attachments[i].finalLayout = i == attachments.size() - 1 ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
		}

		attachments[0].format = vk::Format::eR32G32B32A32Sfloat;
		attachments[1].format = vk::Format::eR16G16B16A16Sfloat;
		attachments[2].format = vk::Format::eR8G8B8A8Unorm;
		attachments[3].format = vk::Format::eR32Sint;
		attachments[4].format = depth_format;

		std::vector<vk::AttachmentReference> color_references;
		color_references.push_back({ 0, vk::ImageLayout::eColorAttachmentOptimal });
		color_references.push_back({ 1, vk::ImageLayout::eColorAttachmentOptimal });
		color_references.push_back({ 2, vk::ImageLayout::eColorAttachmentOptimal });
		color_references.push_back({ 3, vk::ImageLayout::eColorAttachmentOptimal });

		vk::AttachmentReference depth_reference;
		depth_reference.attachment = 4;
		depth_reference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		vk::SubpassDescription subpass;
		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass.pColorAttachments = color_references.data();
		subpass.colorAttachmentCount = color_references.size();
		subpass.pDepthStencilAttachment = &depth_reference;

		std::array<vk::SubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
		dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
		dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

		vk::RenderPassCreateInfo rpci;
		rpci.pAttachments = attachments.data();
		rpci.attachmentCount = attachments.size();
		rpci.subpassCount = 1;
		rpci.pSubpasses = &subpass;
		rpci.dependencyCount = dependencies.size();
		rpci.pDependencies = dependencies.data();

		render_pass = vmc.logical_device.get().createRenderPass(rpci);
    }

    vk::RenderPass RenderPass::get() const
    {
        return render_pass;
    }

    void RenderPass::self_destruct()
    {
        vmc.logical_device.get().destroyRenderPass(render_pass);
    }
} // namespace ve
