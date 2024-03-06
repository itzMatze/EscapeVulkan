#version 460

#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#include "common.glsl"

#define FIREFLY_INTENSITY 100.0

layout(constant_id = 0) const uint NUM_LIGHTS = 1;
layout(constant_id = 1) const uint SEGMENT_COUNT = 1;
layout(constant_id = 2) const uint FIREFLIES_PER_SEGMENT = 1;
layout(constant_id = 3) const uint RESERVOIR_COUNT = 1;
layout(constant_id = 4) const uint RESOLUTION_X = 1;
layout(constant_id = 5) const uint RESOLUTION_Y = 1;
layout(constant_id = 6) const uint FIRST_PASS = 1;
const uint PIXEL_COUNT = RESOLUTION_X * RESOLUTION_Y;

layout(location = 0) in vec2 frag_tex;

layout(location = 0) out vec4 out_color;

layout(binding = 1) buffer MeshRenderDataBuffer {
    MeshRenderData mesh_rd[];
};

layout(binding = 2) uniform sampler2DArray tex_sampler; // textures

layout(binding = 3) buffer material_buffer {
    Material materials[];
};

layout(binding = 4) uniform LightsBuffer {
    Light lights[NUM_LIGHTS];
};

layout(binding = 5) buffer FireflyBuffer {
    AlignedFireflyVertex firefly_vertices[SEGMENT_COUNT * FIREFLIES_PER_SEGMENT];
};

layout(binding = 6) uniform sampler2DArray noise_tex_sampler; // noise textures

layout(binding = 10) buffer TunnelIndicesBuffer {
    uint tunnel_indices[];
};

layout(binding = 11) buffer TunnelVerticesBuffer {
    AlignedTunnelVertex tunnel_vertices[];
};

layout(binding = 12) buffer SceneIndicesBuffer {
    uint scene_indices[];
};

layout(binding = 13) buffer SceneVerticesBuffer {
    AlignedVertex scene_vertices[];
};

layout(binding = 90) uniform FrameDataBuffer {
    FrameData frame_data;
};

layout(binding = 99, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = 100) uniform sampler2D deferred_position_sampler;
layout(binding = 101) uniform sampler2D deferred_normal_sampler;
layout(binding = 102) uniform sampler2D deferred_color_sampler;
layout(binding = 103) uniform isampler2D deferred_segment_uid_sampler;
layout(binding = 104) uniform sampler2D deferred_motion_sampler;

struct Reservoir {
    uint y;
    float w;
    uint M;
    float W;
};

Reservoir local_reservoirs[RESERVOIR_COUNT];

layout(binding = 200) readonly buffer OldRestirReservoirsBuffer
{
    Reservoir old_reservoirs[];
};

layout(binding = 201) writeonly buffer NewRestirReservoirsBuffer
{
    Reservoir new_reservoirs[];
};

Reservoir get_reservoir(in vec2 xy, in uint idx)
{
    return old_reservoirs[idx * PIXEL_COUNT + uint(xy.y) * RESOLUTION_X + uint(xy.x)];
}

void write_reservoir(in Reservoir r, in vec2 xy, in uint idx)
{
    new_reservoirs[idx * PIXEL_COUNT + uint(xy.y) * RESOLUTION_X + uint(xy.x)] = r;
}

uint rng_state;

uint PCGHashState()
{
    rng_state = rng_state * 747796405u + 2891336453u;
    uint state = rng_state;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint PCGHash(uint seed)
{
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float pcg_random_state()
{
    return (float(PCGHashState()) / float(0xFFFFFFFFU));
}

float pcg_random_state_clipped_spatial()
{
    return cos(PI * PCGHashState()) * (((float(PCGHashState()) / float(0xFFFFFFFFU)) * 2.0) + 1.0);
}

float pcg_random(uint seed)
{
    return (float(PCGHash(seed)) / float(0xFFFFFFFFU));
}

vec2 sample_motion(int radius)
{
    ivec2 ipos = ivec2(gl_FragCoord);
    int r = radius;
    float l = -1.0;
    vec2 motion = vec2(0);
    for(int yy = -r; yy <= r; yy++) {
        for(int xx = -r; xx <= r; xx++) {
            int segment_uid = texelFetch(deferred_segment_uid_sampler, ipos + ivec2(xx, yy), 0).x;
            if (segment_uid < 0) continue;
            vec2 m = texelFetch(deferred_motion_sampler, ipos + ivec2(xx, yy), 0).rg;
            if(dot(m, m) > l) {
                motion = m;
                l = dot(m, m);
            }
        }
    }
    return motion * 0.5; // transform from NDC to texture space
}

void update_reservoir(inout Reservoir r, uint x, float weight, uint M)
{
    r.w += weight;
    r.M += M;
    if (pcg_random_state() < (weight / r.w))
    {
        r.y = x;
    }
}

void update_local_reservoirs(uint x, float weight, uint M)
{
    for (uint i = 0; i < RESERVOIR_COUNT; ++i)
    {
        update_reservoir(local_reservoirs[i], x, weight, M);
    }
}

vec3 importance_sample_ggx(vec3 normal, float roughness)
{
    float a = roughness * roughness;
    
    float phi = 2.0 * PI * pcg_random_state();
    uint seed = floatBitsToUint(pcg_random_state());
    float cos_theta = sqrt((1.0 - pcg_random(seed)) / (1.0 + (a * a - 1.0) * pcg_random(seed)));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sin_theta;
    H.y = sin(phi) * sin_theta;
    H.z = cos_theta;
    
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);
    
    vec3 sample_vec = tangent * H.x + bitangent * H.y + normal * H.z;
    return normalize(sample_vec);
}  

bool light_visible(in vec3 ro, in vec3 rd, in float max_t)
{
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, ro, 0.01, rd, max_t);
    rayQueryProceedEXT(rayQuery);
    return !(rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT);
}

bool evaluate_ray(in vec3 ro, in vec3 rd, out float t, out int instance_id, out int geometry_idx, out int primitive_idx, out vec2 bary)
{
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsNoneEXT, 0xFF, ro, 0.01, rd, 10000.0);
    while(rayQueryProceedEXT(rayQuery))
    {
        if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
        {
            t = rayQueryGetIntersectionTEXT(rayQuery, true);
            instance_id = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
            geometry_idx = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true);
            primitive_idx = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
            bary = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
            return true;
        }
    }
    return false;
}

vec4 calculate_cone_light_contribution_with_visibility_check(in vec3 pos, in vec3 normal, in vec4 albedo, in uint i)
{
    vec3 L = normalize(lights[i].pos_inner.xyz - pos);
    if (!light_visible(pos, L, distance(lights[i].pos_inner.xyz, pos))) return vec4(0.0, 0.0, 0.0, 1.0);
    float outer_cone_reduction = pow(clamp((dot(-L, lights[i].dir_intensity.xyz) - lights[i].color_outer.w) / (lights[i].pos_inner.w - lights[i].color_outer.w), 0.0, 1.0), 2);
    return albedo * max(dot(normal, L), 0.0) * outer_cone_reduction * lights[i].dir_intensity.w / pow(distance(lights[i].pos_inner.xyz, pos), 2);
}

vec4 calculate_firefly_light_contribution_with_visibility_check(in vec3 pos, in vec3 normal, in vec4 albedo, in uint i)
{
    vec3 firefly_pos = get_firefly_vertex_pos(firefly_vertices[i]);
    vec3 L = normalize(firefly_pos - pos);
    if (!light_visible(pos, L, distance(firefly_pos, pos))) return vec4(0.0, 0.0, 0.0, 1.0);
    return albedo * max(dot(normal, L), 0.0) * (vec4(get_firefly_vertex_color(firefly_vertices[i]), 1.0) * FIREFLY_INTENSITY) / pow(distance(firefly_pos, pos), 2);
}

vec4 calculate_cone_light_contribution(in vec3 pos, in vec3 normal, in vec4 albedo, in uint i)
{
    vec3 L = normalize(lights[i].pos_inner.xyz - pos);
    float outer_cone_reduction = pow(clamp((dot(-L, lights[i].dir_intensity.xyz) - lights[i].color_outer.w) / (lights[i].pos_inner.w - lights[i].color_outer.w), 0.0, 1.0), 2);
    return albedo * max(dot(normal, L), 0.0) * outer_cone_reduction * lights[i].dir_intensity.w / pow(distance(lights[i].pos_inner.xyz, pos), 2);
}

vec4 calculate_firefly_light_contribution(in vec3 pos, in vec3 normal, in vec4 albedo, in uint i)
{
    vec3 firefly_pos = get_firefly_vertex_pos(firefly_vertices[i]);
    vec3 L = normalize(firefly_pos - pos);
    return albedo * max(dot(normal, L), 0.0) * (vec4(get_firefly_vertex_color(firefly_vertices[i]), 1.0) * FIREFLY_INTENSITY) / pow(distance(firefly_pos, pos), 2);
}

vec4 calculate_light_contribution_with_visibility_check(in vec3 pos, in vec3 normal, in vec4 albedo, in uint i)
{
    if (i >= NUM_LIGHTS) return calculate_firefly_light_contribution_with_visibility_check(pos, normal, albedo, i - NUM_LIGHTS);
    else return calculate_cone_light_contribution_with_visibility_check(pos, normal, albedo, i);
}

vec4 calculate_light_contribution(in vec3 pos, in vec3 normal, in vec4 albedo, in uint i)
{
    if (i >= NUM_LIGHTS) return calculate_firefly_light_contribution(pos, normal, albedo, i - NUM_LIGHTS);
    else return calculate_cone_light_contribution(pos, normal, albedo, i);
}

void fill_reservoirs(in vec3 pos, in vec3 normal, in vec4 albedo, in uint segment_uid)
{
    for (uint i = 0; i < NUM_LIGHTS; ++i)
    {
        float weight = length(calculate_cone_light_contribution(pos, normal, albedo, i).rgb);
        update_local_reservoirs(i, weight, 1);
    }
    uint start_idx = (max(0, (segment_uid - 1)) % SEGMENT_COUNT) * FIREFLIES_PER_SEGMENT;
    uint end_idx = uint(min(start_idx + 3 * FIREFLIES_PER_SEGMENT, FIREFLIES_PER_SEGMENT * SEGMENT_COUNT));
    uint remaining_fireflies = 3 * FIREFLIES_PER_SEGMENT - (end_idx - start_idx);
#if 0
    for (uint i = 0; i < 256; ++i)
    {
        uint rnd_idx = uint(pcg_random_state() * 3.0 * FIREFLIES_PER_SEGMENT);
        if (rnd_idx >= remaining_fireflies) rnd_idx = rnd_idx - remaining_fireflies + start_idx;
        float weight = length(calculate_firefly_light_contribution(pos, normal, albedo, rnd_idx).rgb);
        update_reservoirs(rnd_idx + NUM_LIGHTS, weight, 1);
    }
#else
    // fireflies are overwritten if their segment gets removed
    // first segment of the tunnel does not necessarily correspond to the beginning of the buffer
    // consider only fireflies in the segment of the fragment and neighboring segments
    // this area will sometimes wrap at the end of the buffer
    for (uint i = start_idx; i < end_idx; ++i)
    {
        float weight = length(calculate_firefly_light_contribution(pos, normal, albedo, i).rgb);
        update_local_reservoirs(i + NUM_LIGHTS, weight, 1);
    }
    for (uint i = 0; i < remaining_fireflies; ++i)
    {
        float weight = length(calculate_firefly_light_contribution(pos, normal, albedo, i).rgb);
        update_local_reservoirs(i + NUM_LIGHTS, weight, 1);
    }
    for (uint i = 0; i < RESERVOIR_COUNT; ++i)
    {
        float sample_weight = length(calculate_light_contribution_with_visibility_check(pos, normal, albedo, local_reservoirs[i].y).rgb);
        if (sample_weight < 0.0001)
        {
            local_reservoirs[i].W = 0;
            continue;
        }
        local_reservoirs[i].W = (1.0 / (sample_weight)) * (1.0 / float(local_reservoirs[i].M)) * local_reservoirs[i].w;
    }
#endif
}

void combine_reservoirs(Reservoir r, uint i, in vec3 pos, in vec3 normal, in vec4 albedo)
{
    if (!(r.w > 0.001)) return;
    r.M = min(local_reservoirs[i].M * 5, r.M);
    update_reservoir(local_reservoirs[i], r.y, length(calculate_light_contribution(pos, normal, albedo, r.y).rgb) * r.W * r.M, r.M);
    float sample_weight = length(calculate_light_contribution(pos, normal, albedo, local_reservoirs[i].y).rgb);
    if (sample_weight < 0.0001) return;
    local_reservoirs[i].W = (1.0 / (sample_weight)) * (1.0 / float(local_reservoirs[i].M)) * local_reservoirs[i].w;
}

void add_temporal_reservoirs(in vec3 pos, in vec3 normal, in vec4 albedo)
{
    vec2 motion = sample_motion(1);
    vec2 tex = gl_FragCoord.xy;
    tex.x += motion.x * RESOLUTION_X;
    tex.y += motion.y * RESOLUTION_Y;
    if (tex.x >= RESOLUTION_X || tex.y >= RESOLUTION_Y) return;
    for (uint i = 0; i < RESERVOIR_COUNT; ++i)
    {
        Reservoir r = get_reservoir(tex, i);
        combine_reservoirs(r, i, pos, normal, albedo);
    }
}

void add_spatial_reservoirs(in vec3 pos, in vec3 normal, in vec4 albedo)
{
    for (uint i = 0; i < 5; ++i)
    {
        ivec2 rnd_idx = ivec2(pcg_random_state_clipped_spatial(), pcg_random_state_clipped_spatial());
        vec2 tex = gl_FragCoord.xy + rnd_idx;
        if (tex.x >= RESOLUTION_X || tex.y >= RESOLUTION_Y) continue;
        Reservoir r = get_reservoir(tex, i);
        combine_reservoirs(r, i, pos, normal, albedo);
    }
}

vec4 calculate_phong(in vec3 pos, in vec3 normal, in vec4 color, in int segment_uid)
{
    vec4 out_color = color * 0.05;
    // spotlights
    for (uint i = 0; i < NUM_LIGHTS; ++i)
    {
        out_color += calculate_cone_light_contribution_with_visibility_check(pos, normal, color, i);
    }
    // fireflies
    uint start_idx = (max(0, (segment_uid - 1)) % SEGMENT_COUNT) * FIREFLIES_PER_SEGMENT;
    uint end_idx = uint(min(start_idx + 3 * FIREFLIES_PER_SEGMENT, FIREFLIES_PER_SEGMENT * SEGMENT_COUNT));
    // fireflies are overwritten if their segment gets removed
    // first segment of the tunnel does not necessarily correspond to the beginning of the buffer
    // consider only fireflies in the segment of the fragment and neighboring segments
    // this area will sometimes wrap at the end of the buffer
    for (uint i = start_idx; i < end_idx; ++i)
    {
        out_color += calculate_firefly_light_contribution_with_visibility_check(pos, normal, color, i);
    }
    uint remaining_fireflies = 3 * FIREFLIES_PER_SEGMENT - (end_idx - start_idx);
    for (uint i = 0; i < remaining_fireflies; ++i)
    {
        out_color += calculate_firefly_light_contribution_with_visibility_check(pos, normal, color, i);
    }
    return out_color;
}

const float gaussian[5][5] = {{0.0030, 0.0133, 0.0219, 0.0133, 0.0030},
                        {0.0133, 0.0596, 0.0983, 0.0596, 0.0133},
                        {0.0219, 0.0983, 0.1621, 0.0983, 0.0219},
                        {0.0133, 0.0596, 0.0983, 0.0596, 0.0133},
                        {0.0030, 0.0133, 0.0219, 0.0133, 0.0030}};

vec4 get_blooming_value()
{
    vec4 value = vec4(0.0);
    for (int i = -4; i <= 4; ++i)
    {
        for (int j = -4; j <= 4; ++j)
        {
            if (i == 0 && j == 0) continue;
            int segment_uid = texture(deferred_segment_uid_sampler, frag_tex + vec2(float(i) / float(RESOLUTION_X), float(j) / float(RESOLUTION_Y))).x;
            if (segment_uid < 0)
            {
                value += texture(deferred_color_sampler, frag_tex + vec2(float(i) / float(RESOLUTION_X), float(j) / float(RESOLUTION_Y))) / (float(abs(i*i) + abs(j*j) + 15));// * gaussian[i+2][j+2];
            }
        }
    }
    return value;
}

vec4 calculate_color(vec3 position, vec3 normal, vec4 color, int segment_uid)
{
    vec3 pos = position;
#if 1
    vec4 out_color = vec4(0.0);
    for (uint i = 0; i < RESERVOIR_COUNT; ++i) out_color += calculate_light_contribution_with_visibility_check(pos, normal, color, local_reservoirs[i].y) * local_reservoirs[i].W;
    out_color /= RESERVOIR_COUNT;
    out_color += get_blooming_value();
#else
    vec4 out_color = calculate_phong(pos, normal, color, segment_uid);
#endif
#if 0
    float t = 0.0;
    int instance_id = 0;
    int primitive_idx = 0;
    int geometry_idx = 0;
    vec2 bary = vec2(0.0);
    for (uint i = 0; i < 1; ++i)
    {
        vec3 dir = importance_sample_ggx(n, 0.6);
        if (evaluate_ray(p, dir, t, instance_id, geometry_idx, primitive_idx, bary))
        {
            pos = pos + t * dir;
            if (instance_id == 666)
            {
                TunnelVertex v0 = unpack_tunnel_vertex(tunnel_vertices[tunnel_indices[frame_data.first_segment_indices_idx + primitive_idx * 3]]);
                TunnelVertex v1 = unpack_tunnel_vertex(tunnel_vertices[tunnel_indices[frame_data.first_segment_indices_idx + primitive_idx * 3 + 1]]);
                TunnelVertex v2 = unpack_tunnel_vertex(tunnel_vertices[tunnel_indices[frame_data.first_segment_indices_idx + primitive_idx * 3 + 2]]);
                color = vec4(0.63, 0.32, 0.18, 1.0) * texture(noise_tex_sampler, vec3(v0.tex, 1));
                normal = normalize(v0.normal + texture(noise_tex_sampler, vec3(v0.tex, 0)).rgb - 0.5);
            }
            else
            {
                if (mesh_rd[geometry_idx].mat_idx < 0) return out_color;
                Vertex v0 = unpack_vertex(scene_vertices[scene_indices[mesh_rd[geometry_idx].indices_idx + primitive_idx * 3]]);
                Vertex v1 = unpack_vertex(scene_vertices[scene_indices[mesh_rd[geometry_idx].indices_idx + primitive_idx * 3 + 1]]);
                Vertex v2 = unpack_vertex(scene_vertices[scene_indices[mesh_rd[geometry_idx].indices_idx + primitive_idx * 3 + 2]]);
                Material m = materials[mesh_rd[geometry_idx].mat_idx];
                if (length(m.emission) > 0.0)
                {
                    out_color += (1.0 / (1.0 + t)) * m.emission;
                    return out_color;
                }
                else if (m.base_texture >= 0)
                {
                    color = texture(tex_sampler, vec3(v0.tex, materials[mesh_rd[geometry_idx].mat_idx].base_texture));
                    normal = normalize(v0.normal + v1.normal + v2.normal);
                }
                else return out_color;
            }
            out_color += (1.0 / (1.0 + t)) * calculate_phong(pos, normal, color, segment_uid);
        }
    }
#endif
    return out_color;
}

void main()
{
    vec3 frag_pos = texture(deferred_position_sampler, frag_tex).xyz;
    vec3 frag_normal = texture(deferred_normal_sampler, frag_tex).xyz;
    vec4 frag_color = texture(deferred_color_sampler, frag_tex);
    int frag_segment_uid = texture(deferred_segment_uid_sampler, frag_tex).x;

    rng_state = floatBitsToUint(frag_pos.x * frag_normal.z * frag_tex.t * frame_data.time + float(FIRST_PASS) * 3.54);

    for (uint i = 0; i < RESERVOIR_COUNT; ++i)
    {
        local_reservoirs[i].y = 0;
        local_reservoirs[i].w = 0.0;
        local_reservoirs[i].M = 0;
        local_reservoirs[i].W = 0.0;
    }
    if (frag_segment_uid < 0)
    {
        out_color = frag_color;
        return;
    }
    if (frame_data.normal_view)
    {
        out_color = vec4((frag_normal + 1.0) / 2.0, 1.0);
        return;
    }
    if (frame_data.color_view)
    {
        out_color = frag_color;
        return;
    }
    if (length(frag_normal) < 0.5)
    {
        Reservoir r;
        r.y = 0;
        r.w = 0.0;
        r.M = 0;
        r.W = 0.0;
        for (uint i = 0; i < RESERVOIR_COUNT; ++i) write_reservoir(r, gl_FragCoord.xy, i);
        out_color = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    if (FIRST_PASS == 1)
    {
        fill_reservoirs(frag_pos, frag_normal, frag_color, frag_segment_uid);
        add_temporal_reservoirs(frag_pos, frag_normal, frag_color);
    }
    else
    {
        for (uint i = 0; i < RESERVOIR_COUNT; ++i) local_reservoirs[i] = get_reservoir(gl_FragCoord.xy, i);
        add_spatial_reservoirs(frag_pos, frag_normal, frag_color);
        out_color = calculate_color(frag_pos, frag_normal, frag_color, frag_segment_uid);
    }
    for (uint i = 0; i < RESERVOIR_COUNT; ++i) write_reservoir(local_reservoirs[i], gl_FragCoord.xy, i);
}

