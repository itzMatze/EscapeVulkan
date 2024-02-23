#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec2 frag_tex;

layout(location = 0) out vec4 out_position;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_color;
layout(location = 3) out int out_segment_uid;
layout(location = 4) out vec2 out_motion;

layout(binding = 1) uniform sampler2D tex_sampler;

void main()
{
    out_position = vec4(0.0);
    out_normal = vec4(0.0);
    out_color = texture(tex_sampler, frag_tex);
    out_segment_uid = -1;
    out_motion = vec2(0.0);
}
