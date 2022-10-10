#pragma once

#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "vk/DescriptorSetHandler.hpp"
#include "vk/Pipeline.hpp"
#include "vk/RenderPass.hpp"
#include "vk/Swapchain.hpp"

namespace ve
{
    enum class BufferNames
    {
        Vertex,
        Index
    };

    struct UniformBufferObject {
        glm::mat4 M;
        glm::mat4 VP;
    };

    struct VulkanRenderContext {
        VulkanRenderContext(const VulkanMainContext& vmc) : descriptor_set_handler(vmc), surface_format(choose_surface_format(vmc)), render_pass(vmc, surface_format.format), swapchain(vmc, surface_format, render_pass.get()), pipeline(vmc)
        {
            uniform_buffers = descriptor_set_handler.add_uniform_buffer(frames_in_flight, std::vector<UniformBufferObject>{ubo}, {uint32_t(vmc.queues_family_indices.transfer), uint32_t(vmc.queues_family_indices.graphics)});
            descriptor_set_handler.construct();
            pipeline.construct(render_pass.get(), descriptor_set_handler);
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

        UniformBufferObject ubo{
                glm::mat4(1.0f),
                glm::mat4(1.0f)};

        const uint32_t frames_in_flight = 2;
        uint32_t current_frame = 0;
        DescriptorSetHandler descriptor_set_handler;
        std::vector<ve::Buffer> uniform_buffers;
        std::unordered_map<BufferNames, Buffer> buffers;
        vk::SurfaceFormatKHR surface_format;
        RenderPass render_pass;
        Swapchain swapchain;
        Pipeline pipeline;

        void recreate_swapchain()
        {
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
    };
}// namespace ve