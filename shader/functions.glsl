#define FIREFLY_INTENSITY 3.0
vec4 calculate_phong(in vec3 normal, in vec4 color, in int segment_uid)
{
    vec4 out_color = color * 0.05;
    // spotlights
    for (uint i = 0; i < NUM_LIGHTS; ++i)
    {
        vec3 L = normalize(lights[i].pos_inner.xyz - frag_pos);
        float outer_cone_reduction = pow(clamp((dot(-L, lights[i].dir_intensity.xyz) - lights[i].color_outer.w) / (lights[i].pos_inner.w - lights[i].color_outer.w), 0.0, 1.0), 2);
        out_color += max(dot(normal, L), 0.0) * outer_cone_reduction * color * (lights[i].dir_intensity.w / (lights[i].dir_intensity.w + pow(distance(lights[i].pos_inner.xyz, frag_pos), 2)));
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
        vec3 L = normalize(firefly_pos - frag_pos);
        out_color += max(dot(normal, L), 0.0) * vec4(get_firefly_vertex_color(firefly_vertices[i]), 1.0) * (FIREFLY_INTENSITY / (FIREFLY_INTENSITY + pow(distance(firefly_pos, frag_pos), 2)));
    }
    uint remaining_fireflies = 3 * FIREFLIES_PER_SEGMENT - (end_idx - start_idx);
    for (uint i = 0; i < remaining_fireflies; ++i)
    {
        vec3 firefly_pos = get_firefly_vertex_pos(firefly_vertices[i]);
        vec3 L = normalize(firefly_pos - frag_pos);
        out_color += max(dot(normal, L), 0.0) * vec4(get_firefly_vertex_color(firefly_vertices[i]), 1.0) * (FIREFLY_INTENSITY / (FIREFLY_INTENSITY + pow(distance(firefly_pos, frag_pos), 2)));
    }
    return out_color;
}
