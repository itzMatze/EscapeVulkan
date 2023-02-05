#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/RenderPass.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"

namespace ve
{
    struct DrawInfo {
        float time_diff = 0.000001f;
		float frametime = 0.0f;
        bool show_ui = true;
    };

    class UI
    {
	public:
		UI(const VulkanMainContext& vmc, const RenderPass& render_pass, uint32_t frames);
        void self_destruct();
		void upload_font_textures(const VulkanCommandContext& vcc);
		void draw(vk::CommandBuffer& cb, DrawInfo& di);
	private:
        const VulkanMainContext& vmc;
		vk::DescriptorPool imgui_pool;
	};
}// namespace ve
