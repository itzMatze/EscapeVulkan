#define PI 3.1415926535897932384626433832

struct PushConstants {
    uint mesh_render_data_idx;
    uint first_segment_indices_idx;
    float time;
    bool tex_view;
};

struct LightingPassPushConstants {
    uint first_segment_indices_idx;
    float time;
    bool normal_view;
    bool color_view;
    bool segment_uid_view;
};

struct NewSegmentPushConstants {
    vec3 p0;
    vec3 p1;
    vec3 p2;
    uint indices_start_idx;
    uint segment_uid;
};

struct FireflyMovePushConstants {
    float time;
    float time_diff;
    uint segment_uid;
    uint first_segment_indices_idx;
};

struct JetParticleMovePushConstants {
    vec3 move_dir;
    float time;
    float time_diff;
};

struct DebugPushConstants {
    mat4 mvp;
};

struct MeshRenderData {
    int model_render_data_idx;
    int mat_idx;
    int indices_idx;
};

struct ModelRenderData {
    mat4 mvp;
    mat4 prev_mvp;
    mat4 m;
    int segment_uid;
};

struct Light {
    vec4 dir_intensity;
    vec4 pos_inner;
    vec4 color_outer;
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

struct BoundingBox
{
    vec3 min_p;
    vec3 max_p;
};

struct ModelMatrices
{
    mat4 m;
    mat4 inv_m;
};

struct PlayerData
{
    vec4 pos;
    vec4 dir;
    vec4 up;
};

struct AlignedVertex {
    vec4 pos_normal_x;
    vec4 normal_yz_color_rg;
    vec4 color_ba_tex;
};

vec3 get_vertex_pos(in AlignedVertex v) { return v.pos_normal_x.xyz; }
void set_vertex_pos(inout AlignedVertex v, in vec3 pos) { v.pos_normal_x.xyz = pos; }

vec3 get_vertex_normal(in AlignedVertex v) { return vec3(v.pos_normal_x.w, v.normal_yz_color_rg.xy); }
void set_vertex_normal(inout AlignedVertex v, in vec3 normal) {
    v.pos_normal_x.w = normal.x;
    v.normal_yz_color_rg.xy = normal.yz;
}

vec4 get_vertex_color(in AlignedVertex v) { return vec4(v.normal_yz_color_rg.zw, v.color_ba_tex.xy); }
void set_vertex_color(inout AlignedVertex v, in vec4 color) {
    v.normal_yz_color_rg.zw = color.xy;
    v.color_ba_tex.xy = color.zw;
}

vec2 get_vertex_tex(in AlignedVertex v) { return v.color_ba_tex.zw; }
void set_vertex_tex(inout AlignedVertex v, in vec2 tex) { v.color_ba_tex.zw = tex; }

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

vec3 get_firefly_vertex_pos(in AlignedFireflyVertex v) { return v.pos_col_r.xyz; }
void set_firefly_vertex_pos(inout AlignedFireflyVertex v, in vec3 pos) { v.pos_col_r.xyz = pos; }

vec3 get_firefly_vertex_color(in AlignedFireflyVertex v) { return vec3(v.pos_col_r.w, v.col_gb_vel_xy.xy); }
void set_firefly_vertex_color(inout AlignedFireflyVertex v, in vec3 color) {
    v.pos_col_r.w = color.x;
    v.col_gb_vel_xy.xy = color.yz;
}

vec3 get_firefly_vertex_vel(in AlignedFireflyVertex v) { return vec3(v.col_gb_vel_xy.zw, v.vel_z_acc.x); }
void set_firefly_vertex_vel(inout AlignedFireflyVertex v, in vec3 vel) {
    v.col_gb_vel_xy.zw = vel.xy;
    v.vel_z_acc.x = vel.z;
}

vec3 get_firefly_vertex_acc(in AlignedFireflyVertex v) { return v.vel_z_acc.yzw; }
void set_firefly_vertex_acc(inout AlignedFireflyVertex v, in vec3 acc) { v.vel_z_acc.yzw = acc; }

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

struct AlignedJetParticleVertex {
    vec4 pos_col_r;
    vec4 col_gb_vel_xy;
    vec2 vel_z_lifetime;
};

vec3 get_jet_particle_vertex_pos(in AlignedJetParticleVertex v) { return v.pos_col_r.xyz; }
void set_jet_particle_vertex_pos(inout AlignedJetParticleVertex v, in vec3 pos) { v.pos_col_r.xyz = pos; }

vec3 get_jet_particle_vertex_color(in AlignedJetParticleVertex v) { return vec3(v.pos_col_r.w, v.col_gb_vel_xy.xy); }
void set_jet_particle_vertex_color(inout AlignedJetParticleVertex v, in vec3 color) {
    v.pos_col_r.w = color.x;
    v.col_gb_vel_xy.xy = color.yz;
}

vec3 get_jet_particle_vertex_vel(in AlignedJetParticleVertex v) { return vec3(v.col_gb_vel_xy.zw, v.vel_z_lifetime.x); }
void set_jet_particle_vertex_vel(inout AlignedJetParticleVertex v, in vec3 vel) {
    v.col_gb_vel_xy.zw = vel.xy;
    v.vel_z_lifetime.x = vel.z;
}

float get_jet_particle_vertex_lifetime(in AlignedJetParticleVertex v) { return v.vel_z_lifetime.y; }
void set_jet_particle_vertex_lifetime(inout AlignedJetParticleVertex v, in float lifetime) { v.vel_z_lifetime.y = lifetime; }

struct JetParticleVertex {
    vec3 pos;
    vec3 col;
    vec3 vel;
    float lifetime;
};

JetParticleVertex unpack_jet_particle_vertex(AlignedJetParticleVertex v)
{
    JetParticleVertex vert;
    vert.pos = v.pos_col_r.xyz;
    vert.col.r = v.pos_col_r.w;
    vert.col.gb = v.col_gb_vel_xy.xy;
    vert.vel.xy = v.col_gb_vel_xy.zw;
    vert.vel.z = v.vel_z_lifetime.x;
    vert.lifetime = v.vel_z_lifetime.y;
    return vert;
}

AlignedJetParticleVertex pack_jet_particle_vertex(JetParticleVertex v)
{
    AlignedJetParticleVertex vert;
    vert.pos_col_r.xyz = v.pos;
    vert.pos_col_r.w = v.col.r;
    vert.col_gb_vel_xy.xy = v.col.gb;
    vert.col_gb_vel_xy.zw = v.vel.xy;
    vert.vel_z_lifetime.x = v.vel.z;
    vert.vel_z_lifetime.y = v.lifetime;
    return vert;
}

struct AlignedTunnelVertex {
    vec4 pos_normal_x;
    vec4 normal_y_tex_xy_segment_uid;
};

vec3 get_tunnel_vertex_pos(in AlignedTunnelVertex v) { return v.pos_normal_x.xyz; }
void set_tunnel_vertex_pos(inout AlignedTunnelVertex v, in vec3 pos) { v.pos_normal_x.xyz = pos; }

vec3 get_tunnel_vertex_normal(in AlignedTunnelVertex v) { return vec3(v.pos_normal_x.w, v.normal_y_tex_xy_segment_uid.x, sign(v.normal_y_tex_xy_segment_uid.w) * sqrt(1.0 - v.pos_normal_x.w * v.pos_normal_x.w - v.normal_y_tex_xy_segment_uid.x * v.normal_y_tex_xy_segment_uid.x)); }
void set_tunnel_vertex_normal(inout AlignedTunnelVertex v, in vec3 normal) {
    v.pos_normal_x.w = normal.x;
    v.normal_y_tex_xy_segment_uid.x = normal.y;
    if (sign(normal.z) != sign(v.normal_y_tex_xy_segment_uid.w)) v.normal_y_tex_xy_segment_uid.w *= -1;
}

vec2 get_tunnel_vertex_tex(in AlignedTunnelVertex v) { return v.normal_y_tex_xy_segment_uid.yz; }
void set_tunnel_vertex_tex(inout AlignedTunnelVertex v, in vec2 tex) { v.normal_y_tex_xy_segment_uid.yz = tex; }

uint get_tunnel_vertex_segment_uid(in AlignedTunnelVertex v) { return uint(abs(v.normal_y_tex_xy_segment_uid.w) + 0.1); }
void set_tunnel_vertex_segment_uid(inout AlignedTunnelVertex v, in uint segment_uid, in vec3 normal) {
    v.normal_y_tex_xy_segment_uid.w = (sign(normal.z) < 0.0) ? -float(segment_uid) : float(segment_uid);
}

struct TunnelVertex {
    vec3 pos;
    vec3 normal;
    vec2 tex;
    uint segment_uid;
};

TunnelVertex unpack_tunnel_vertex(AlignedTunnelVertex v)
{
    TunnelVertex vert;
    vert.pos = v.pos_normal_x.xyz;
    vert.normal.x = v.pos_normal_x.w;
    vert.normal.y = v.normal_y_tex_xy_segment_uid.x;
    vert.normal.z = sign(v.normal_y_tex_xy_segment_uid.w) * sqrt(1.0 - vert.normal.x * vert.normal.x - vert.normal.y * vert.normal.y);
    vert.tex = v.normal_y_tex_xy_segment_uid.yz;
    vert.segment_uid = uint(abs(v.normal_y_tex_xy_segment_uid.w) + 0.1);
    return vert;
}

AlignedTunnelVertex pack_tunnel_vertex(TunnelVertex v)
{
    AlignedTunnelVertex vert;
    vert.pos_normal_x.xyz= v.pos;
    vert.pos_normal_x.w = v.normal.x;
    vert.normal_y_tex_xy_segment_uid.x = v.normal.y;
    vert.normal_y_tex_xy_segment_uid.yz = v.tex;
    vert.normal_y_tex_xy_segment_uid.w = (sign(v.normal.z) < 0.0) ? -float(v.segment_uid) : float(v.segment_uid);
    return vert;
}
