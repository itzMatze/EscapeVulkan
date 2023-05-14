#define PI 3.1415926535897932384626433832

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

struct NewSegmentPushConstants {
    vec3 p0;
    vec3 p1;
    vec3 p2;
    uint indices_start_idx;
    uint segment_idx;
};

struct FireflyMovePushConstants {
    float time;
    float time_diff;
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

AlignedVertex pack_vertex(Vertex v)
{
    AlignedVertex vert;
    vert.pos_normal_x.xyz = v.pos;
    vert.pos_normal_x.w = v.normal.x;
    vert.normal_yz_color_rg.xy = v.normal.yz;
    vert.normal_yz_color_rg.zw = v.color.rg;
    vert.color_ba_tex.xy = v.color.ba;
    vert.color_ba_tex.zw = v.tex;
    return vert;
}

struct AlignedFireflyVertex {
    vec4 pos_col_r;
    vec4 col_gb_vel_xy;
    vec4 vel_z_acc;
};

struct FireflyVertex {
    vec3 pos;
    vec3 col;
    vec3 vel;
    vec3 acc;
};

FireflyVertex unpack_firefly_vertex(AlignedFireflyVertex v)
{
    FireflyVertex vert;
    vert.pos = v.pos_col_r.xyz;
    vert.col.r = v.pos_col_r.w;
    vert.col.gb = v.col_gb_vel_xy.xy;
    vert.vel.xy = v.col_gb_vel_xy.zw;
    vert.vel.z = v.vel_z_acc.x;
    vert.acc = v.vel_z_acc.yzw;
    return vert;
}

AlignedFireflyVertex pack_firefly_vertex(FireflyVertex v)
{
    AlignedFireflyVertex vert;
    vert.pos_col_r.xyz = v.pos;
    vert.pos_col_r.w = v.col.r;
    vert.col_gb_vel_xy.xy = v.col.gb;
    vert.col_gb_vel_xy.zw = v.vel.xy;
    vert.vel_z_acc.x = v.vel.z;
    vert.vel_z_acc.yzw = v.acc;
    return vert;
}
