#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(constant_id = 0) const uint NUM_LIGHTS = 1;
layout(constant_id = 1) const uint SEGMENT_COUNT = 1;
layout(constant_id = 2) const uint FIREFLIES_PER_SEGMENT = 1;

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec4 frag_color;
layout(location = 3) in vec2 frag_tex;
layout(location = 4) flat in int frag_segment_uid;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstant {
    PushConstants pc;
};

layout(binding = 2) uniform sampler2DArray tex_sampler;

layout(binding = 3) buffer material_buffer {
    Material materials[];
};

layout(binding = 4) uniform LightsBuffer {
    Light lights[NUM_LIGHTS];
};

layout(binding = 5) buffer FireflyBuffer {
    AlignedFireflyVertex firefly_vertices[SEGMENT_COUNT * FIREFLIES_PER_SEGMENT];
};

#include "functions.glsl"

void main()
{
    if (pc.normal_view)
    {
        out_color = vec4((frag_normal + 1.0) / 2.0, 1.0);
        return;
    }
    if (pc.tex_view)
    {
        out_color = vec4(frag_tex, 1.0f, 1.0f);
        return;
    }

    vec4 texture_color = texture(tex_sampler, vec3(frag_tex, materials[pc.mat_idx].base_texture));
    out_color = calculate_phong(frag_normal, texture_color, frag_segment_uid);
}
