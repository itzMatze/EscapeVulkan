#version 460

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_color;
layout(location = 2) in vec2 frag_tex;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = max(0.01, dot(normalize(vec3(1.0, 1.0, -1.0)), frag_normal)) * vec4(frag_color, 1.0);
}
