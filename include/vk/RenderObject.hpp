#pragma once

#include "vk/Model.hpp"
#include "vk/Pipeline.hpp"
#include "vk/RenderPass.hpp"
#include "vk/common.hpp"

namespace ve
{
    // groups rendering of models that use the same pipeline together
    class RenderObject
    {
    public:
        RenderObject(const VulkanMainContext& vmc);
        void self_destruct();
        void add_model_meshes(std::vector<Mesh>& mesh_list);
        void add_bindings(VulkanStorageContext& vsc, const std::vector<Material>& materials);
        void construct(const RenderPass& render_pass, const std::vector<ShaderInfo>& shader_names);
        void draw(vk::CommandBuffer& cb, DrawInfo& di);

        DescriptorSetHandler dsh;

    private:
        const VulkanMainContext& vmc;
        // store meshes and the indices where a different model begins
        std::vector<Mesh> meshes;
        std::vector<uint32_t> model_indices;
        std::vector<uint32_t> descriptor_set_indices;
        Pipeline pipeline;
        Pipeline mesh_view_pipeline;
    };
} // namespace ve
