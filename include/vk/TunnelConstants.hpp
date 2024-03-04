#include <cstdint>

namespace ve
{
    constexpr float segment_scale = 20.0f;
    constexpr uint32_t segment_count = 16; // how many segments are in the tunnel (must be power of two)
    static_assert((segment_count & (segment_count - 1)) == 0);
    constexpr uint32_t samples_per_segment = 32; // how many sample rings one segment is made of
    constexpr uint32_t vertices_per_sample = 360; // how many vertices are sampled in one sample ring
    constexpr uint32_t vertex_count = segment_count * samples_per_segment * vertices_per_sample;
    // two triangles per vertex on a sample (3 indices per triangle); every sample of a segment except the last one has triangles
    constexpr uint32_t indices_per_segment = (samples_per_segment - 1) * vertices_per_sample * 6;
    constexpr uint32_t index_count = indices_per_segment * segment_count;
    constexpr uint32_t fireflies_per_segment = 15;
    constexpr uint32_t firefly_count = fireflies_per_segment * segment_count;
    constexpr uint32_t reservoir_count = 4;
    constexpr uint32_t player_local_segment_position = 2;

} // namespace ve

