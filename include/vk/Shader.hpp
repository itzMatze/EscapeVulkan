#pragma once

#include <fstream>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "ve_log.hpp"

namespace ve
{
    class Shader
    {
    public:
        Shader(const vk::Device& device, const std::string filename,
               vk::ShaderStageFlagBits shader_stage_flag) : name(filename), device(device)
        {
            VE_LOG_CONSOLE(VE_INFO, "Loading shader \"" << filename << "\"\n");
            std::string source = read_shader_file(std::string("shader/" + filename + ".spv"));
            vk::ShaderModuleCreateInfo smci{};
            smci.sType = vk::StructureType::eShaderModuleCreateInfo;
            smci.codeSize = source.size();
            smci.pCode = reinterpret_cast<const uint32_t*>(source.c_str());
            shader_module = device.createShaderModule(smci);

            pssci.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
            pssci.stage = shader_stage_flag;
            pssci.module = shader_module;
            pssci.pName = "main";
        }

        void self_destruct()
        {
            device.destroyShaderModule(shader_module);
        }

        const vk::ShaderModule get() const
        {
            return shader_module;
        }

        const vk::PipelineShaderStageCreateInfo& get_stage_create_info() const
        {
            return pssci;
        }

    private:
        std::string read_shader_file(const std::string& filename)
        {
            std::ifstream file(filename, std::ios::binary);
            VE_ASSERT(file.is_open(), "Failed to open shader file " << filename);
            std::ostringstream file_stream;
            file_stream << file.rdbuf();
            return file_stream.str();
        }

        const std::string name;
        const vk::Device& device;
        vk::ShaderModule shader_module;
        vk::PipelineShaderStageCreateInfo pssci;
    };
}// namespace ve