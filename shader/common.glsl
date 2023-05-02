struct PushConstant {
    uint mvp_idx;
    int mat_idx;
    uint light_count;
    bool normal_view;
};

struct UBO {
    mat4 mvp;
    mat4 m;
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
