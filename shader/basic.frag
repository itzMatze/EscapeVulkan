#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec4 frag_color;
layout(location = 2) in vec2 frag_tex;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstant
{
    PushConstants pc;
};

void main()
{
    if (pc.normal_view)
    {
        out_color = vec4((frag_normal + 1.0) / 2.0, 1.0);
        return;
    }
    out_color = max(0.1, dot(normalize(vec3(1.0, 1.0, -1.0)), frag_normal) * 0.8) * frag_color;
}
