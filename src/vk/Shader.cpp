#include "vk/Shader.hpp"

#include <fstream>
#include <iostream>
#include <vector>

#include "ve_log.hpp"

namespace ve
{
    Shader::Shader(const vk::Device& device, const std::string filename,
                   vk::ShaderStageFlagBits shader_stage_flag) : name(filename), device(device)
    {
        spdlog::info("Loading shader \"{}\"", filename);
        std::string source = read_shader_file(std::string("../shader/bin/" + filename + ".spv"));
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

    void Shader::self_destruct()
    {
        device.destroyShaderModule(shader_module);
    }

    const vk::ShaderModule Shader::get() const
    {
        return shader_module;
    }

    const vk::PipelineShaderStageCreateInfo& Shader::get_stage_create_info() const
    {
        return pssci;
    }

    std::string Shader::read_shader_file(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        VE_ASSERT(file.is_open(), "Failed to open shader file \"{}\"", filename);
        std::ostringstream file_stream;
        file_stream << file.rdbuf();
        return file_stream.str();
    }
}// namespace ve
