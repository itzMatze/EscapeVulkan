#version 460

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 tex;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_tex;

layout(binding = 0) uniform UniformBufferObject {
    mat4 M;
    mat4 VP;
} ubo;

void main() {
    gl_Position = ubo.VP * ubo.M * vec4(pos, 1.0);
    frag_color = color;
    frag_tex = tex;
}
