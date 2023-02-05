#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/RenderPass.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"

namespace ve
{
    class UI
    {
	public:
		UI(const VulkanMainContext& vmc, const RenderPass& render_pass, uint32_t frames);
        void self_destruct();
		void upload_font_textures(const VulkanCommandContext& vcc);
		void draw(vk::CommandBuffer& cb);
	private:
        const VulkanMainContext& vmc;
		vk::DescriptorPool imgui_pool;
	};
}// namespace ve
