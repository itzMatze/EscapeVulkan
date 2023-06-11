#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(constant_id = 0) const uint NUM_MVPS = 1;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex;
layout(location = 3) in int segment_uid;

layout(location = 0) out vec3 frag_pos;
layout(location = 1) flat out vec3 frag_normal;
layout(location = 2) out vec2 frag_tex;
layout(location = 3) flat out int frag_segment_uid;

layout(binding = 0) uniform ModelRenderDataBuffer {
    ModelRenderData mrd[NUM_MVPS];
};

layout(push_constant) uniform PushConstant {
    PushConstants pc;
};

void main() {
    gl_Position = mrd[pc.mvp_idx].mvp * vec4(pos, 1.0);
    frag_pos = vec3(mrd[pc.mvp_idx].m * vec4(pos, 1.0));
    frag_normal = normal;
    frag_tex = tex;
    frag_segment_uid = segment_uid;
}
