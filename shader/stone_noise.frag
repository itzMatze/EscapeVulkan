#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(constant_id = 0) const uint NUM_LIGHTS = 1;

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec4 frag_color;
layout(location = 3) in vec2 frag_tex;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstant
{
    PushConstants pc;
};

layout(binding = 1) uniform sampler2DArray tex_sampler;

layout(binding = 4) uniform LightsBuffer {
    Light lights[NUM_LIGHTS];
};

void main()
{
    if (pc.tex_view)
    {
        out_color = vec4(frag_tex, 1.0f, 1.0f);
        return;
    }

    // read displacement of normal from noise texture
    vec3 normal = normalize(frag_normal + texture(tex_sampler, vec3(frag_tex, 0)).rgb - 0.5);

    if (pc.normal_view)
    {
        out_color = vec4((normal + 1.0) / 2.0, 1.0);
        return;
    }

    // add noise from noise texture to color
    vec3 color = vec3(0.63, 0.32, 0.18) * texture(tex_sampler, vec3(frag_tex, 1)).rgb;

    vec3 ambient = color * 0.05;
    vec3 diffuse = vec3(0.0, 0.0, 0.0);

    // phong shading 
    for (uint i = 0; i < pc.light_count; ++i)
    {
        vec3 L = normalize(lights[i].pos_inner.xyz - frag_pos);
        float outer_cone_reduction = pow(clamp((dot(-L, lights[i].dir_intensity.xyz) - lights[i].color_outer.w) / (lights[i].pos_inner.w - lights[i].color_outer.w), 0.0, 1.0), 2);
        diffuse += max(dot(normal, L), 0.0) * outer_cone_reduction * color * (lights[i].dir_intensity.w / (lights[i].dir_intensity.w + pow(distance(lights[i].pos_inner.xyz, frag_pos), 2)));
    }
    out_color = vec4(ambient + diffuse, 1.0);
}
