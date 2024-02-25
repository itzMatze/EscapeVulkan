#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(constant_id = 0) const uint NUM_MVPS = 1;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 normal;
layout(location = 2) in vec2 tex;
layout(location = 3) in float segment_uid;

layout(location = 0) out vec3 frag_pos;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_tex;
layout(location = 3) flat out int frag_segment_uid;
layout(location = 4) out vec4 prev_cs_frag_pos;
layout(location = 5) out vec4 cs_frag_pos;

layout(binding = 0) uniform ModelRenderDataBuffer {
    ModelRenderData mrd;
};

layout(push_constant) uniform PushConstant {
    RenderPushConstants pc;
};

void main() {
    prev_cs_frag_pos = mrd.prev_mvp * vec4(pos, 1.0);
    cs_frag_pos = mrd.mvp * vec4(pos, 1.0);
    gl_Position = mrd.mvp * vec4(pos, 1.0);
    frag_pos = pos;
    frag_normal = vec3(normal, sign(segment_uid) * sqrt(1.0 - normal.x * normal.x - normal.y * normal.y));
    frag_tex = tex;
    frag_segment_uid = int(abs(segment_uid) + 0.1);
}
