#version 460

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

layout(location = 0) out vec3 frag_color;

layout(binding = 0) uniform UniformBufferObject {
    mat4 M;
    mat4 VP;
} ubo;

void main() {
    gl_Position = ubo.VP * ubo.M * vec4(pos, 1.0);
    frag_color = col;
}
