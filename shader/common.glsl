struct PushConstants {
    uint mvp_idx;
    int mat_idx;
    uint light_count;
    float time;
    bool normal_view;
    bool tex_view;
};

struct ModelRenderData {
    mat4 mvp;
    mat4 m;
};

struct Light {
    vec4 dir_intensity;
    vec4 pos_inner;
    vec4 color_outer;
};

struct ComputePushConstants {
    vec3 p0;
    vec3 p1;
    vec3 p2;
    uint indices_start_idx;
    uint segment_idx;
};

struct Material {
    vec4 base_color;
    vec4 emission;
    float metallic;
    float roughness;
    int base_texture;
    //int metallic_roughness_texture;
    //int normal_texture;
    //int occlusion_texture;
    //int emissive_texture;
};

struct AlignedVertex {
    vec4 pos_normal_x;
    vec4 normal_yz_color_rg;
    vec4 color_ba_tex;
};

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec4 color;
    vec2 tex;
};

Vertex unpack_vertex(AlignedVertex v)
{
    Vertex vert;
    vert.pos = v.pos_normal_x.xyz;
    vert.normal.x = v.pos_normal_x.w;
    vert.normal.yz = v.normal_yz_color_rg.xy;
    vert.color.rg = v.normal_yz_color_rg.zw;
    vert.color.ba = v.color_ba_tex.xy;
    vert.tex = v.color_ba_tex.zw;
    return vert;
}
