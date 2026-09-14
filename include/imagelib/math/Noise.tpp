#ifndef IMAGELIB_MATH_NOISE_H_
#error "Include imagelib/math/Noise.h, not this .tpp directly."
#endif

namespace iml {
namespace math {
namespace noise {

/// Fractional Brownian motion over a binary noise function.
/// `baseFn(seed, x, y)` returns base noise such that the FBM in [-1,1] space.
template <typename Fn>
float fbmGeneric(uint64 seed, float x, float y, const FractalParams& p, Fn baseFn) noexcept {
    float amp  = p.gain;
    float freq = p.frequency;
    float sum  = 0.0f;
    float norm = 0.0f;
    for (int o = 0; o < p.octaves; ++o) {
        sum += baseFn(seed, x * freq, y * freq) * amp;
        norm += amp;
        freq *= p.lacunarity;
        amp  *= p.persistence;
    }
    return sum / norm;
}

} // namespace noise
} // namespace math
} // namespace iml