#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec3 frag_color;
layout(location = 3) in vec2 frag_tex;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstants
{
    PushConstant pc;
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
    out_color = materials[pc.mat_idx].emission;
}
