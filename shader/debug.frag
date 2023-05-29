#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec4 frag_color;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = frag_color;
}
