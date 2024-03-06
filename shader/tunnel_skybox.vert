#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;

layout(location = 0) out vec3 frag_pos;
layout(location = 1) out vec2 frag_tex;

layout(push_constant) uniform PushConstant {
    TunnelSkyboxPushConstants pc;
};

void main() {
    gl_Position = pc.mvp * vec4(pos, 1.0);
    frag_tex = tex;
}
