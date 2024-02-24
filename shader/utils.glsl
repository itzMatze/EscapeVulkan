// rotate v around k by angle degrees
vec3 rotate(vec3 v, vec3 k, float angle)
{
    float cos_theta = cos(radians(angle));
    float sin_theta = sin(radians(angle));
    vec3 rotated = (v * cos_theta) + (cross(k, v) * sin_theta) + (k * dot(k, v)) * (1 - cos_theta);
    return rotated;
}

