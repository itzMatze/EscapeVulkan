#version 460 core

layout(location = 0) in vec3 in_color;
layout(location = 0) out vec4 out_color;

void main() {
    if (pow(gl_PointCoord.x - 0.5, 2) + pow(gl_PointCoord.y - 0.5, 2) > pow(0.5, 2)) discard;
    else if (pow(gl_PointCoord.x - 0.5, 2) + pow(gl_PointCoord.y - 0.5, 2) > pow(0.25, 2)) out_color = vec4(in_color * (0.5 - (pow(gl_PointCoord.x - 0.5, 2) + pow(gl_PointCoord.y - 0.5, 2) - pow(0.25, 2)) * 2), 1.0);
    else out_color = vec4(in_color, 1.0);
}
