#version 460

#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#include "common.glsl"

layout(constant_id = 0) const uint NUM_LIGHTS = 1;
layout(constant_id = 1) const uint SEGMENT_COUNT = 1;
layout(constant_id = 2) const uint FIREFLIES_PER_SEGMENT = 1;

layout(location = 0) in vec2 frag_tex;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstant {
    LightingPassPushConstants pc;
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

layout(binding = 99, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = 100) uniform sampler2D deferred_position_sampler;
layout(binding = 101) uniform sampler2D deferred_normal_sampler;
layout(binding = 102) uniform sampler2D deferred_color_sampler;
layout(binding = 103) uniform isampler2D deferred_segment_uid_sampler;

#include "functions.glsl"

void main()
{
    vec3 frag_pos = texture(deferred_position_sampler, frag_tex).xyz;
    vec3 frag_normal = texture(deferred_normal_sampler, frag_tex).xyz;
    vec4 frag_color = texture(deferred_color_sampler, frag_tex);
    int frag_segment_uid = texture(deferred_segment_uid_sampler, frag_tex).x;

    rng_state = floatBitsToUint(frag_pos.x * frag_normal.z * frag_tex.t * pc.time);
    if (pc.normal_view)
    {
        out_color = vec4((frag_normal + 1.0) / 2.0, 1.0);
        return;
    }
    if (pc.color_view)
    {
        out_color = frag_color;
        return;
    }
    if (frag_segment_uid < 0)
    {
        out_color = frag_color;
    }
    else
    {
        out_color = calculate_color(frag_pos, frag_normal, frag_color, frag_segment_uid);
    }
}

