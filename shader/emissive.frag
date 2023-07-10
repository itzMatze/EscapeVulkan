#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec4 frag_color;
layout(location = 3) in vec2 frag_tex;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstant {
    PushConstants pc;
};

layout(binding = 1) buffer MeshRenderDataBuffer {
    MeshRenderData mesh_rd[];
};

layout(binding = 3) buffer material_buffer {
    Material materials[];
};

void main()
{
    if (pc.normal_view)
    {
        out_color = vec4((frag_normal + 1.0) / 2.0, 1.0);
        return;
    }
    if (pc.tex_view)
    {
        out_color = vec4(frag_tex, 1.0f, 1.0f);
        return;
    }
    out_color = materials[mesh_rd[pc.mesh_render_data_idx].mat_idx].emission;
}
