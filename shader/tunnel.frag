#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(constant_id = 0) const uint NUM_LIGHTS = 1;
layout(constant_id = 1) const uint SEGMENT_COUNT = 1;
layout(constant_id = 2) const uint FIREFLIES_PER_SEGMENT = 1;

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec2 frag_tex;
layout(location = 3) flat in int frag_segment_uid;
layout(location = 4) in vec4 prev_cs_frag_pos;
layout(location = 5) in vec4 cs_frag_pos;

layout(location = 0) out vec4 out_position;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_color;
layout(location = 3) out int out_segment_uid;
layout(location = 4) out vec2 out_motion;

layout(push_constant) uniform PushConstant {
    TunnelPushConstants pc;
};

layout(binding = 1) buffer MeshRenderDataBuffer {
    MeshRenderData mesh_rd[];
};

layout(binding = 2) uniform sampler2DArray tex_sampler; // textures

layout(binding = 3) buffer material_buffer {
    Material materials[];
};

layout(binding = 4) uniform LightsBuffer {
    Light lights[NUM_LIGHTS];
};

layout(binding = 5) buffer FireflyBuffer {
    AlignedFireflyVertex firefly_vertices[SEGMENT_COUNT * FIREFLIES_PER_SEGMENT];
};

layout(binding = 6) uniform sampler2DArray noise_tex_sampler; // noise textures

layout(binding = 10) buffer TunnelIndicesBuffer {
    uint tunnel_indices[];
};

layout(binding = 11) buffer TunnelVerticesBuffer {
    AlignedTunnelVertex tunnel_vertices[];
};

layout(binding = 12) buffer SceneIndicesBuffer {
    uint scene_indices[];
};

layout(binding = 13) buffer SceneVerticesBuffer {
    AlignedVertex scene_vertices[];
};

void main()
{
    // read displacement of normal from noise texture
    vec3 normal = normalize(frag_normal + texture(noise_tex_sampler, vec3(frag_tex, 0)).rgb - 0.5);

    if (pc.tex_view)
    {
        out_color = vec4(frag_tex, 1.0, 1.0);
        out_segment_uid = -1;
        return;
    }

    {
        // add noise from noise texture to color
        vec4 color = vec4(0.63, 0.32, 0.18, 1.0) * texture(noise_tex_sampler, vec3(frag_tex, 1));

        out_position = vec4(frag_pos, 1.0);
        out_normal = vec4(normal, 1.0);
        out_color = color;
        out_segment_uid = frag_segment_uid;
        out_motion = prev_cs_frag_pos.xy / prev_cs_frag_pos.w - cs_frag_pos.xy / cs_frag_pos.w;
    }
}
