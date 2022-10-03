#pragma once

#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"
#include "vk/RenderPass.hpp"
#include "vk/Shader.hpp"

namespace ve
{
    class Pipeline
    {
    public:
        Pipeline(const LogicalDevice& logical_device, const vk::RenderPass render_pass) : device(logical_device.get())
        {
            VE_LOG_CONSOLE(PINK << "Creating pipeline");
            std::vector<Shader> shader_group;
            shader_group.push_back(Shader(device, "default.vert", vk::ShaderStageFlagBits::eVertex));
            shader_group.push_back(Shader(device, "default.frag", vk::ShaderStageFlagBits::eFragment));

            std::vector<vk::PipelineShaderStageCreateInfo> shader_stage;
            for (const auto& shader: shader_group)
            {
                shader_stage.push_back(shader.get_stage_create_info());
            }

            std::vector<vk::DynamicState> dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
            vk::PipelineDynamicStateCreateInfo pdsci{};
            pdsci.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
            pdsci.dynamicStateCount = dynamic_states.size();
            pdsci.pDynamicStates = dynamic_states.data();

            vk::PipelineVertexInputStateCreateInfo pvisci{};
            pvisci.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
            pvisci.vertexBindingDescriptionCount = 0;
            pvisci.pVertexBindingDescriptions = nullptr;
            pvisci.vertexAttributeDescriptionCount = 0;
            pvisci.pVertexAttributeDescriptions = nullptr;

            vk::PipelineInputAssemblyStateCreateInfo piasci{};
            piasci.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
            piasci.topology = vk::PrimitiveTopology::eTriangleList;
            piasci.primitiveRestartEnable = VK_FALSE;

            /*
// used they have no dynamic state
            vk::Viewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = swapchain.get_extent().width;
            viewport.height = swapchain.get_extent().height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            vk::Rect2D scissor{};
            scissor.offset = vk::Offset2D(0, 0);
            scissor.extent = swapchain.get_extent();
*/

            vk::PipelineViewportStateCreateInfo pvsci{};
            pvsci.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
            pvsci.viewportCount = 1;
            pvsci.scissorCount = 1;

            vk::PipelineRasterizationStateCreateInfo prsci{};
            prsci.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
            prsci.depthClampEnable = VK_FALSE;
            prsci.rasterizerDiscardEnable = VK_FALSE;
            prsci.polygonMode = vk::PolygonMode::eFill;
            prsci.lineWidth = 1.0f;
            prsci.cullMode = vk::CullModeFlagBits::eBack;
            prsci.frontFace = vk::FrontFace::eClockwise;
            prsci.depthBiasEnable = VK_FALSE;
            prsci.depthBiasConstantFactor = 0.0f;
            prsci.depthBiasClamp = 0.0f;
            prsci.depthBiasSlopeFactor = 0.0f;

            vk::PipelineMultisampleStateCreateInfo pmssci{};
            pmssci.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
            pmssci.sampleShadingEnable = VK_FALSE;
            pmssci.rasterizationSamples = vk::SampleCountFlagBits::e1;
            pmssci.minSampleShading = 1.0f;
            pmssci.pSampleMask = nullptr;
            pmssci.alphaToCoverageEnable = VK_FALSE;
            pmssci.alphaToOneEnable = VK_FALSE;

            vk::PipelineColorBlendAttachmentState pcbas{};
            pcbas.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            pcbas.blendEnable = VK_FALSE;
            pcbas.srcColorBlendFactor = vk::BlendFactor::eOne;
            pcbas.dstColorBlendFactor = vk::BlendFactor::eZero;
            pcbas.colorBlendOp = vk::BlendOp::eAdd;
            pcbas.srcAlphaBlendFactor = vk::BlendFactor::eOne;
            pcbas.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            pcbas.alphaBlendOp = vk::BlendOp::eAdd;

            vk::PipelineColorBlendStateCreateInfo pcbsci{};
            pcbsci.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
            pcbsci.logicOpEnable = VK_FALSE;
            pcbsci.logicOp = vk::LogicOp::eCopy;
            pcbsci.attachmentCount = 1;
            pcbsci.pAttachments = &pcbas;
            pcbsci.blendConstants[0] = 0.0f;
            pcbsci.blendConstants[1] = 0.0f;
            pcbsci.blendConstants[2] = 0.0f;
            pcbsci.blendConstants[3] = 0.0f;

            vk::PipelineLayoutCreateInfo plci{};
            plci.sType = vk::StructureType::ePipelineLayoutCreateInfo;
            plci.setLayoutCount = 0;
            plci.pSetLayouts = nullptr;
            plci.pushConstantRangeCount = 0;
            plci.pPushConstantRanges = nullptr;

            VE_LOG_CONSOLE("Creating pipeline layout");
            pipeline_layout = device.createPipelineLayout(plci);

            vk::GraphicsPipelineCreateInfo gpci{};
            gpci.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
            gpci.stageCount = 2;
            gpci.pStages = shader_stage.data();
            gpci.pVertexInputState = &pvisci;
            gpci.pInputAssemblyState = &piasci;
            gpci.pViewportState = &pvsci;
            gpci.pRasterizationState = &prsci;
            gpci.pMultisampleState = &pmssci;
            gpci.pDepthStencilState = nullptr;
            gpci.pColorBlendState = &pcbsci;
            gpci.pDynamicState = &pdsci;
            gpci.layout = pipeline_layout;
            gpci.renderPass = render_pass;
            gpci.subpass = 0;
            // it is possible to create a new pipeline by deriving from an existing one
            gpci.basePipelineHandle = VK_NULL_HANDLE;
            gpci.basePipelineIndex = -1;

            VE_LOG_CONSOLE("Creating pipeline");
            vk::ResultValue<vk::Pipeline> pipeline_result_value = device.createGraphicsPipeline(VK_NULL_HANDLE, gpci);
            VE_CHECK(pipeline_result_value.result, "Failed to create pipeline!");
            pipeline = pipeline_result_value.value;

            for (auto& shader: shader_group)
            {
                shader.self_destruct(device);
            }
        }

        ~Pipeline()
        {
            VE_LOG_CONSOLE(PINK << "Destroying pipeline");
            device.destroyPipeline(pipeline);
            VE_LOG_CONSOLE("Destroying pipeline layout");
            device.destroyPipelineLayout(pipeline_layout);
        }

    private:
        const vk::Device device;
        vk::PipelineLayout pipeline_layout;
        vk::Pipeline pipeline;
    };
}// namespace ve
