#version 450 core

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(constant_id = 0) const uint NUM_MVPS = 1;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
//layout(location = 2) in vec3 vel;
//layout(location = 3) in vec3 acc;

layout(binding = 0) uniform ModelRenderDataBuffer {
    ModelRenderData mrd[NUM_MVPS];
};

layout(push_constant) uniform PushConstant {
    PushConstants pc;
};

layout(location = 0) out vec3 frag_color;


void main() {
    vec4 clip_pos = mrd[pc.mvp_idx].mvp * vec4(pos, 1.0);
    gl_PointSize = 500.0 / clip_pos.w;
    gl_Position = clip_pos;

    frag_color = col;
}
