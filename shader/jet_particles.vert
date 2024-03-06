#version 460 core

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(constant_id = 0) const float POINT_SIZE = 1.0f;
layout(constant_id = 1) const float MAX_LIFETIME = 1.0f;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 3) in float lifetime;

layout(binding = 0) uniform ModelRenderDataBuffer {
    ModelRenderData mrd;
};

layout(location = 0) out vec3 frag_color;

void main() {
    vec4 clip_pos = mrd.mvp * vec4(pos, 1.0);
    gl_PointSize = (POINT_SIZE - ((MAX_LIFETIME - lifetime) * POINT_SIZE / MAX_LIFETIME)) / clip_pos.w;
    gl_Position = clip_pos;

    frag_color = col;
}
