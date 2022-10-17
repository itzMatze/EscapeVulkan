#version 460

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 tex;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_tex;

layout(binding = 0) uniform UniformBufferObject {
    mat4 VP;
} ubo;

layout(push_constant) uniform PushConstants
{
    mat4 MVP;
} pc;

void main() {
    gl_Position = pc.MVP * vec4(pos, 1.0);
    frag_color = color;
    frag_tex = tex;
}
