#define FIREFLY_INTENSITY 3.0

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
    return ((float(PCGHashState()) / float(0xFFFFFFFFU)) - 0.5) * 2.0;
}

float pcg_random(uint seed)
{
    return ((float(PCGHash(seed)) / float(0xFFFFFFFFU)) - 0.5) * 2.0;
}

float pcg_random_state_clipped()
{
    return cos(PI * PCGHashState()) * (((float(PCGHashState()) / float(0xFFFFFFFFU)) * 0.5) + 0.5);
}

float pcg_random_clipped(uint seed)
{
    return cos(PI * PCGHashState()) * (((float(PCGHash(seed)) / float(0xFFFFFFFFU)) * 0.5) + 0.5);
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
    rayQueryProceedEXT(rayQuery);
    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
    {
        t = rayQueryGetIntersectionTEXT(rayQuery, true);
        instance_id = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
        geometry_idx = rayQueryGetIntersectionGeometryIndexEXT(rayQuery, true);
        primitive_idx = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
        bary = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
        return true;
    }
    return false;
}

vec4 calculate_phong(in vec3 pos, in vec3 normal, in vec4 color, in int segment_uid)
{
    vec4 out_color = color * 0.05;
    // spotlights
    for (uint i = 0; i < NUM_LIGHTS; ++i)
    {
        vec3 L = normalize(lights[i].pos_inner.xyz - pos);
        if (light_visible(pos, L, distance(lights[i].pos_inner.xyz, pos)))
        {
            float outer_cone_reduction = pow(clamp((dot(-L, lights[i].dir_intensity.xyz) - lights[i].color_outer.w) / (lights[i].pos_inner.w - lights[i].color_outer.w), 0.0, 1.0), 2);
            out_color += max(dot(normal, L), 0.0) * outer_cone_reduction * (lights[i].dir_intensity.w / (lights[i].dir_intensity.w + pow(distance(lights[i].pos_inner.xyz, pos), 2)));
        }
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
        vec3 firefly_pos = get_firefly_vertex_pos(firefly_vertices[i]);
        vec3 L = normalize(firefly_pos - pos);
        if (light_visible(pos, L, distance(firefly_pos, pos)))
        {
            out_color += max(dot(normal, L), 0.0) * vec4(get_firefly_vertex_color(firefly_vertices[i]), 1.0) * (FIREFLY_INTENSITY / (FIREFLY_INTENSITY + pow(distance(firefly_pos, pos), 2)));
        }
    }
    uint remaining_fireflies = 3 * FIREFLIES_PER_SEGMENT - (end_idx - start_idx);
    for (uint i = 0; i < remaining_fireflies; ++i)
    {
        vec3 firefly_pos = get_firefly_vertex_pos(firefly_vertices[i]);
        vec3 L = normalize(firefly_pos - pos);
        if (light_visible(pos, L, distance(firefly_pos, pos)))
        {
            out_color += max(dot(normal, L), 0.0) * vec4(get_firefly_vertex_color(firefly_vertices[i]), 1.0) * (FIREFLY_INTENSITY / (FIREFLY_INTENSITY + pow(distance(firefly_pos, pos), 2)));
        }
    }
    return color * out_color;
}

vec4 calculate_color(vec3 position, vec3 normal, vec4 color, int segment_uid)
{
    vec3 n = normal;
    vec3 pos = position;
    vec3 p = pos;
    vec4 out_color = calculate_phong(pos, normal, color, segment_uid);
    float t = 0.0;
    int instance_id = 0;
    int primitive_idx = 0;
    int geometry_idx = 0;
    vec2 bary = vec2(0.0);
    for (uint i = 0; i < 0; ++i)
    {
        vec3 dir = importance_sample_ggx(n, 0.6);
        if (evaluate_ray(p, dir, t, instance_id, geometry_idx, primitive_idx, bary))
        {
            pos = pos + t * dir;
            if (instance_id == 666)
            {
                TunnelVertex v0 = unpack_tunnel_vertex(tunnel_vertices[tunnel_indices[pc.first_segment_indices_idx + primitive_idx * 3]]);
                TunnelVertex v1 = unpack_tunnel_vertex(tunnel_vertices[tunnel_indices[pc.first_segment_indices_idx + primitive_idx * 3 + 1]]);
                TunnelVertex v2 = unpack_tunnel_vertex(tunnel_vertices[tunnel_indices[pc.first_segment_indices_idx + primitive_idx * 3 + 2]]);
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
    return out_color * 2.0;
}
