#version 460

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

#define OCTAVES 4

layout(constant_id = 0) const uint NUM_LIGHTS = 1;

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec4 frag_color;
layout(location = 3) in vec2 frag_tex;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstant
{
    PushConstants pc;
};

layout(binding = 4) uniform LightsBuffer {
    Light lights[NUM_LIGHTS];
};

// noise functions taken from https://thebookofshaders.com
float random(vec2 st){
    st = vec2(dot(st,vec2(127.1, 311.7)), dot(st,vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec2 random2(vec2 p) {
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)))) * 43758.5453);
}

// noise for fbm function 
float noise(in vec2 st) 
{
    vec2 i = floor(st);
    vec2 f = fract(st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return ((mix(a, b, u.x) + (c - a)* u.y * (1.0 - u.x) + (d - b) * u.x * u.y) + 0.5) / 1.5;
}

// fractional brownian motion for color
float fbm(in vec2 st) 
{
    // Initial values
    float value = 0.0;
    float amplitud = 0.5;
    float frequency = 0.;
    //
    // Loop of octaves
    for (int i = 0; i < OCTAVES; i++) 
    {
        value += amplitud * noise(st);
        st *= 2.0;
        amplitud *= 0.5;
    }
    return value;
}

// fractional brownian motion for normal
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }

float snoise(vec2 v) 
{
    // Precompute values for skewed triangular grid
    const vec4 C = vec4(0.211324865405187, 0.366025403784439, -0.577350269189626, 0.024390243902439);

    // First corner (x0)
    vec2 i  = floor(v + dot(v, C.yy));
    vec2 x0 = v - i + dot(i, C.xx);

    // Other two corners (x1, x2)
    vec2 i1 = vec2(0.0);
    i1 = (x0.x > x0.y)? vec2(1.0, 0.0):vec2(0.0, 1.0);
    vec2 x1 = x0.xy + C.xx - i1;
    vec2 x2 = x0.xy + C.zz;

    // Do some permutations to avoid truncation effects in permutation
    i = mod289(i);
    vec3 p = permute(permute( i.y + vec3(0.0, i1.y, 1.0)) + i.x + vec3(0.0, i1.x, 1.0 ));

    vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x1,x1), dot(x2,x2)), 0.0);

    m = m*m;
    m = m*m;

    // Gradients:
    //  41 pts uniformly over a line, mapped onto a diamond
    //  The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)

    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;

    // Normalise gradients implicitly by scaling m
    // Approximation of: m *= inversesqrt(a0*a0 + h*h);
    m *= 1.79284291400159 - 0.85373472095314 * (a0*a0+h*h);

    // Compute final noise value at P
    vec3 g = vec3(0.0);
    g.x  = a0.x  * x0.x  + h.x  * x0.y;
    g.yz = a0.yz * vec2(x1.x,x2.x) + h.yz * vec2(x1.y,x2.y);
    return 130.0 * dot(m, g);
}

float turbulence(in vec2 st) 
{
    // Initial values
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 0.;
    //
    // Loop of octaves
    for (int i = 0; i < OCTAVES; i++) 
    {
        value += amplitude * abs(snoise(st));
        st *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// worley noise
float cellular(vec2 p) 
{
    vec2 i_st = floor(p);
    vec2 f_st = fract(p);
    float m_dist = 10.;
    for (int j=-1; j<=1; j++ ) 
    {
        for (int i=-1; i<=1; i++ ) 
        {
            vec2 neighbor = vec2(float(i),float(j));
            vec2 point = random2(i_st + neighbor);
            point = 0.5 + 0.5*sin(6.2831*point);
            vec2 diff = neighbor + point - f_st;
            float dist = length(diff);
            if( dist < m_dist ) 
            {
                m_dist = dist;
            }
        }
    }
    return m_dist;
}

void main()
{
    // mix worley noise and brownian noise to get a stone texture
    vec3 normal_displacement = vec3(turbulence(frag_tex * 80.0 + 0.1), turbulence(frag_tex * 80.0 + 0.23), turbulence(frag_tex * 80.0 + 0.69));
    normal_displacement *= 1.0 - vec3(cellular(frag_tex * 320.0 + 0.1), cellular(frag_tex * 320.0 + 0.23), cellular(frag_tex * 320.0 + 0.69));
    vec3 normal = normalize(frag_normal + normal_displacement - 0.5);

    if (pc.normal_view)
    {
        out_color = vec4((normal + 1.0) / 2.0, 1.0);
        return;
    }
    if (pc.tex_view)
    {
        out_color = vec4(frag_tex, 1.0f, 1.0f);
        return;
    }

    vec3 color = vec3(0.63, 0.32, 0.18);

    // add noise to color
    color.r *= (fbm(frag_tex*10.0) + 1.0) / 2.0;
    color.g *= (fbm(frag_tex*10.0) + 1.0) / 2.0;
    color.b *= (fbm(frag_tex*10.0) + 1.0) / 2.0;

    vec3 ambient = color * 0.01;
    vec3 diffuse = vec3(0.0, 0.0, 0.0);
    
    // phong shading 
    for (uint i = 0; i < pc.light_count; ++i)
    {
        vec3 L = normalize(lights[i].pos_inner.xyz - frag_pos);
        float outer_cone_reduction = pow(clamp((dot(-L, lights[i].dir_intensity.xyz) - lights[i].color_outer.w) / (lights[i].pos_inner.w - lights[i].color_outer.w), 0.0, 1.0), 2);
        diffuse += max(dot(normal, L), 0.0) * outer_cone_reduction * color * (lights[i].dir_intensity.w / (lights[i].dir_intensity.w + pow(distance(lights[i].pos_inner.xyz, frag_pos), 2)));
    }
    out_color = vec4(ambient + diffuse, 1.0);
}
