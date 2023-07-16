#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(constant_id = 0) const uint NUM_MVPS = 1;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in vec2 tex;

layout(location = 0) out vec3 frag_pos;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec4 frag_color;
layout(location = 3) out vec2 frag_tex;
layout(location = 4) out int frag_segment_uid;
layout(location = 5) out vec4 prev_cs_frag_pos;
layout(location = 6) out vec4 cs_frag_pos;

layout(binding = 0) uniform ModelRenderDataBuffer {
    ModelRenderData mrd[NUM_MVPS];
};

layout(binding = 1) buffer MeshRenderDataBuffer {
    MeshRenderData mesh_rd[];
};

layout(push_constant) uniform PushConstant {
    PushConstants pc;
};

void main() {
    prev_cs_frag_pos = mrd[mesh_rd[pc.mesh_render_data_idx].model_render_data_idx].prev_mvp * vec4(pos, 1.0);
    cs_frag_pos = mrd[mesh_rd[pc.mesh_render_data_idx].model_render_data_idx].mvp * vec4(pos, 1.0);
    gl_Position = mrd[mesh_rd[pc.mesh_render_data_idx].model_render_data_idx].mvp * vec4(pos, 1.0);
    frag_pos = vec3(mrd[mesh_rd[pc.mesh_render_data_idx].model_render_data_idx].m * vec4(pos, 1.0));
    frag_normal = normal;
    frag_color = color;
    frag_tex = tex;
    frag_segment_uid = mrd[mesh_rd[pc.mesh_render_data_idx].model_render_data_idx].segment_uid;
}
