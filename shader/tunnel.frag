#version 460

#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#include "common.glsl"

layout(constant_id = 0) const uint NUM_LIGHTS = 1;
layout(constant_id = 1) const uint SEGMENT_COUNT = 1;
layout(constant_id = 2) const uint FIREFLIES_PER_SEGMENT = 1;

layout(location = 0) in vec3 frag_pos;
layout(location = 1) flat in vec3 frag_normal;
layout(location = 2) in vec2 frag_tex;
layout(location = 3) flat in int frag_segment_uid;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstant {
    PushConstants pc;
};

layout(binding = 1) uniform sampler2DArray tex_sampler;

layout(binding = 2, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = 4) uniform LightsBuffer {
    Light lights[NUM_LIGHTS];
};

layout(binding = 5) buffer FireflyBuffer {
    AlignedFireflyVertex firefly_vertices[SEGMENT_COUNT * FIREFLIES_PER_SEGMENT];
};

#include "functions.glsl"

void main()
{
    // read displacement of normal from noise texture
    vec3 normal = normalize(frag_normal + texture(tex_sampler, vec3(frag_tex, 0)).rgb - 0.5);

    if (pc.normal_view)
    {
        out_color = vec4((normal + 1.0) / 2.0, 1.0);
        return;
    }
    if (pc.tex_view)
    {
        out_color = vec4(frag_tex, 1.0, 1.0);
        return;
    }

    {
        // add noise from noise texture to color
        vec4 color = vec4(0.63, 0.32, 0.18, 1.0) * texture(tex_sampler, vec3(frag_tex, 1));

        out_color = calculate_phong(normal, color, frag_segment_uid);
        /*
        out_color = vec4(0.0, 0.0, 0.0, 1.0);
        rayQueryEXT rayQuery;
        rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsNoneEXT, 0xFF, frag_pos, 0.01, vec3(1.0, 0.0, 0.0), 1000.0);
        while (rayQueryProceedEXT(rayQuery));
        if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
        {
            out_color += color * calculate_phong(normal, color, frag_segment_uid);
        }*/
    }
}
