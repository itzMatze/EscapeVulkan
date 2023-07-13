#include "vk/Pipeline.hpp"

#include "ve_log.hpp"
#include "vk/RenderPass.hpp"
#include "vk/common.hpp"

namespace ve
{
    Pipeline::Pipeline(const VulkanMainContext& vmc) : vmc(vmc)
    {}

    void Pipeline::self_destruct()
    {
        vmc.logical_device.get().destroyPipeline(pipeline);
        vmc.logical_device.get().destroyPipelineLayout(pipeline_layout);
    }

    void Pipeline::construct(const RenderPass& render_pass, std::optional<vk::DescriptorSetLayout> set_layout, const std::vector<ShaderInfo>& shader_infos, vk::PolygonMode polygon_mode, const std::vector<vk::VertexInputBindingDescription>& binding_descriptions, const std::vector<vk::VertexInputAttributeDescription>& attribute_description, const vk::PrimitiveTopology& primitive_topology, const std::vector<vk::PushConstantRange>& pcrs)
    {
        std::vector<Shader> shaders;
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
        for (const auto& shader_info : shader_infos)
        {
            Shader shader(vmc.logical_device.get(), shader_info.shader_name, shader_info.stage_flag);
            shaders.push_back(shader);
            vk::PipelineShaderStageCreateInfo pssci = shader.get_stage_create_info();
            pssci.pSpecializationInfo = &shader_info.spec_info;
            shader_stages.push_back(pssci);
        }

        std::vector<vk::DynamicState> dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo pdsci{};
        pdsci.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
        pdsci.dynamicStateCount = dynamic_states.size();
        pdsci.pDynamicStates = dynamic_states.data();

        vk::PipelineVertexInputStateCreateInfo pvisci{};
        pvisci.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
        pvisci.vertexBindingDescriptionCount = binding_descriptions.size();
        pvisci.pVertexBindingDescriptions = binding_descriptions.data();
        pvisci.vertexAttributeDescriptionCount = attribute_description.size();
        pvisci.pVertexAttributeDescriptions = attribute_description.data();

        vk::PipelineInputAssemblyStateCreateInfo piasci{};
        piasci.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
        piasci.topology = primitive_topology;
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
        prsci.polygonMode = polygon_mode;
        prsci.lineWidth = 0.5f;
        prsci.cullMode = vk::CullModeFlagBits::eNone;
        prsci.frontFace = vk::FrontFace::eCounterClockwise;
        prsci.depthBiasEnable = VK_FALSE;
        prsci.depthBiasConstantFactor = 0.0f;
        prsci.depthBiasClamp = 0.0f;
        prsci.depthBiasSlopeFactor = 0.0f;

        vk::PipelineMultisampleStateCreateInfo pmssci{};
        pmssci.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
        pmssci.sampleShadingEnable = VK_TRUE;
        pmssci.rasterizationSamples = vk::SampleCountFlagBits::e1;
        pmssci.minSampleShading = 0.4f;
        pmssci.pSampleMask = nullptr;
        pmssci.alphaToCoverageEnable = VK_FALSE;
        pmssci.alphaToOneEnable = VK_FALSE;

        std::vector<vk::PipelineColorBlendAttachmentState> pcbas(render_pass.attachment_count);
        for (uint32_t i = 0; i < render_pass.attachment_count; ++i)
        {
            pcbas[i].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            pcbas[i].blendEnable = VK_FALSE;
            pcbas[i].srcColorBlendFactor = vk::BlendFactor::eOne;
            pcbas[i].dstColorBlendFactor = vk::BlendFactor::eZero;
            pcbas[i].colorBlendOp = vk::BlendOp::eAdd;
            pcbas[i].srcAlphaBlendFactor = vk::BlendFactor::eOne;
            pcbas[i].dstAlphaBlendFactor = vk::BlendFactor::eZero;
            pcbas[i].alphaBlendOp = vk::BlendOp::eAdd;
        }

        vk::PipelineColorBlendStateCreateInfo pcbsci{};
        pcbsci.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
        pcbsci.logicOpEnable = VK_FALSE;
        pcbsci.logicOp = vk::LogicOp::eCopy;
        pcbsci.attachmentCount = pcbas.size();
        pcbsci.pAttachments = pcbas.data();
        pcbsci.blendConstants[0] = 0.0f;
        pcbsci.blendConstants[1] = 0.0f;
        pcbsci.blendConstants[2] = 0.0f;
        pcbsci.blendConstants[3] = 0.0f;

        vk::PipelineLayoutCreateInfo plci{};
        plci.sType = vk::StructureType::ePipelineLayoutCreateInfo;
        if (set_layout.has_value())
        {
            plci.setLayoutCount = 1;
            plci.pSetLayouts = &set_layout.value();
        }
        plci.pushConstantRangeCount = pcrs.size();
        plci.pPushConstantRanges = pcrs.data();

        pipeline_layout = vmc.logical_device.get().createPipelineLayout(plci);

        vk::PipelineDepthStencilStateCreateInfo pdssci{};
        pdssci.sType = vk::StructureType::ePipelineDepthStencilStateCreateInfo;
        pdssci.depthTestEnable = VK_TRUE;
        pdssci.depthWriteEnable = VK_TRUE;
        pdssci.depthCompareOp = vk::CompareOp::eLess;
        pdssci.depthBoundsTestEnable = VK_FALSE;
        pdssci.minDepthBounds = 0.0f;
        pdssci.maxDepthBounds = 1.0f;
        pdssci.stencilTestEnable = VK_FALSE;
        pdssci.front = vk::StencilOpState{};
        pdssci.back = vk::StencilOpState{};

        vk::GraphicsPipelineCreateInfo gpci{};
        gpci.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
        gpci.stageCount = shader_stages.size();
        gpci.pStages = shader_stages.data();
        gpci.pVertexInputState = &pvisci;
        gpci.pInputAssemblyState = &piasci;
        gpci.pViewportState = &pvsci;
        gpci.pRasterizationState = &prsci;
        gpci.pMultisampleState = &pmssci;
        gpci.pDepthStencilState = &pdssci;
        gpci.pColorBlendState = &pcbsci;
        gpci.pDynamicState = &pdsci;
        gpci.layout = pipeline_layout;
        gpci.renderPass = render_pass.get();
        gpci.subpass = 0;
        // it is possible to create a new pipeline by deriving from an existing one
        gpci.basePipelineHandle = VK_NULL_HANDLE;
        gpci.basePipelineIndex = -1;

        vk::ResultValue<vk::Pipeline> pipeline_result_value = vmc.logical_device.get().createGraphicsPipeline(VK_NULL_HANDLE, gpci);
        VE_CHECK(pipeline_result_value.result, "Failed to create pipeline!");
        pipeline = pipeline_result_value.value;

        for (auto& shader : shaders) shader.self_destruct();
    }

    void Pipeline::construct(vk::DescriptorSetLayout set_layout, const ShaderInfo& shader_info, uint32_t push_constant_byte_size)
    {
        Shader shader(vmc.logical_device.get(), shader_info.shader_name, vk::ShaderStageFlagBits::eCompute);

        vk::PushConstantRange pcr;
        pcr.offset = 0;
        pcr.size = push_constant_byte_size;
        pcr.stageFlags = vk::ShaderStageFlagBits::eCompute;

        vk::PipelineLayoutCreateInfo plci{};
        plci.sType = vk::StructureType::ePipelineLayoutCreateInfo;
        plci.setLayoutCount = 1;
        plci.pSetLayouts = &set_layout;
        if (push_constant_byte_size > 0)
        {
            plci.pushConstantRangeCount = 1;
            plci.pPushConstantRanges = &pcr;
        }

        pipeline_layout = vmc.logical_device.get().createPipelineLayout(plci);

        vk::PipelineShaderStageCreateInfo pssci = shader.get_stage_create_info();
        pssci.pSpecializationInfo = &shader_info.spec_info;

        vk::ComputePipelineCreateInfo cpci{};
        cpci.sType = vk::StructureType::eComputePipelineCreateInfo;
        cpci.stage = pssci;
        cpci.layout = pipeline_layout;

        vk::ResultValue<vk::Pipeline> comute_pipeline_result_value = vmc.logical_device.get().createComputePipeline(VK_NULL_HANDLE, cpci);
        VE_CHECK(comute_pipeline_result_value.result, "Failed to create compute pipeline!");
        pipeline = comute_pipeline_result_value.value;

        shader.self_destruct();
    }

    const vk::Pipeline& Pipeline::get() const
    {
        return pipeline;
    }

    const vk::PipelineLayout& Pipeline::get_layout() const
    {
        return pipeline_layout;
    }
} // namespace ve
