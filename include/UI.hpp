#pragma once

#include <vulkan/vulkan.hpp>

#include "vk/common.hpp"
#include "vk/RenderPass.hpp"
#include "vk/VulkanMainContext.hpp"
#include "vk/VulkanCommandContext.hpp"
#include "FixVector.hpp"

namespace ve
{
    class UI
    {
	public:
		UI(const VulkanMainContext& vmc, const RenderPass& render_pass, uint32_t frames);
        void self_destruct();
		void upload_font_textures(VulkanCommandContext& vcc);
		void draw(vk::CommandBuffer& cb, GameState& gs);
	private:
        const VulkanMainContext& vmc;
		vk::DescriptorPool imgui_pool;
        FixVector<float> frametime_values;
        float time_diff = 0.0f;
        float frametime = 0.0f;
        std::vector<FixVector<float>> devicetiming_values;
        std::vector<float> devicetimings;
	};
} // namespace ve
