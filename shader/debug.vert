#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;

layout(location = 0) out vec3 frag_pos;
layout(location = 1) out vec4 frag_color;

layout(push_constant) uniform DebugPushConstant {
    DebugPushConstants pc;
};

void main() {
    gl_Position = pc.mvp * vec4(pos, 1.0);
    frag_color = color;
}
