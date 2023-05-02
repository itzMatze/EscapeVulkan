#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec4 frag_color;
layout(location = 3) in vec2 frag_tex;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstant
{
    PushConstants pc;
};

layout(binding = 4) buffer light_buffer {
    Light lights[];
};

void main()
{
    if (pc.normal_view)
    {
        out_color = vec4((frag_normal + 1.0) / 2.0, 1.0);
        return;
    }

    vec4 ambient = frag_color * 0.1;
    vec4 diffuse = vec4(0.0, 0.0, 0.0, 0.0);
    
    for (uint i = 0; i < pc.light_count; ++i)
    {
        vec3 L = normalize(lights[i].pos_inner.xyz - frag_pos);
        float outer_cone_reduction = pow(clamp((dot(-L, lights[i].dir_intensity.xyz) - lights[i].color_outer.w) / (lights[i].pos_inner.w - lights[i].color_outer.w), 0.0, 1.0), 2);
        diffuse += max(dot(frag_normal, L), 0.0) * outer_cone_reduction * frag_color * (lights[i].dir_intensity.w / (lights[i].dir_intensity.w + pow(distance(lights[i].pos_inner.xyz, frag_pos), 2)));
    }
    out_color = ambient + diffuse;
}
