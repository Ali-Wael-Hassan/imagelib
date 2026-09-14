#pragma once
// imagelib/procedural/Noise.h
//
// Rendering mathematical noise (iml::math::noise) into images. Every render
// writes a scalar into the destination: grayscale layouts get channel 0,
// multichannel layouts get a monochrome (v,v,v) value. All noise is sampled in
// normalized [0,1] and passed through iml::proc::detail::writeRGBA so any
// storage type (UInt8/16/32/Float32) works.

#include "imagelib/core/Image.h"
#include "imagelib/math/Noise.h"
#include "imagelib/procedural/Detail.h"

#include <cstdint>

namespace iml {
namespace procgen {

enum class NoiseType : uint8 {
    White = 0,    // per-lattice white hash
    Value,        // interpolated lattice hashes
    Perlin,       // classic gradient lattice
    Simplex,      // 2D simplex noise
    Worley,       // F1 cellular distance
    WorleyEdge,   // F2-F1 cellular "crackle"
    Fbm,          // fractal brownian motion
    Ridged,       // ridged multifractal
    Billow,       // abs() lobes
    Hybrid,       // hybrid multifractal
    DomainWarp,   // smudged manifolds
};

struct NoiseSettings {
    NoiseType                          type      = NoiseType::Perlin;
    uint64                             seed      = 0;
    float                              frequency = 1.0f;
    math::noise::FractalParams         params;
};

namespace detail {

inline float sampleNoise(NoiseType type, uint64 seed, float x, float y, float frequency,
                         const math::noise::FractalParams& p) noexcept {
    const float fx = x * frequency;
    const float fy = y * frequency;
    switch (type) {
        default:
        case NoiseType::White:
            return math::noise::whiteNoise2D(seed, fx, fy);
        case NoiseType::Value:
            return math::noise::valueNoise2D(seed, fx, fy);
        case NoiseType::Perlin:
            return math::noise::gradientNoise2D(seed, fx, fy);
        case NoiseType::Simplex:
            return math::noise::simplexNoise2D(seed, fx, fy);
        case NoiseType::Worley:
            return proc::detail::c01(math::noise::worley2D(seed, fx, fy).value());
        case NoiseType::WorleyEdge:
            return proc::detail::c01(math::noise::worley2D(seed, fx, fy).edge());
        case NoiseType::Fbm:
            return math::noise::fbm2D(seed, fx, fy, p) * 0.5f + 0.5f;
        case NoiseType::Ridged:
            return math::noise::ridged2D(seed, fx, fy, p);
        case NoiseType::Billow:
            return math::noise::billow2D(seed, fx, fy, p);
        case NoiseType::Hybrid:
            return math::noise::hybrid2D(seed, fx, fy, p) * 0.5f + 0.5f;
        case NoiseType::DomainWarp:
            return math::noise::domainWarp2D(seed, fx, fy, 1.0f, p) * 0.5f + 0.5f;
    }
}

} // namespace detail

/// Render a scalar noise field into `dst`.
inline void fillNoise(ImageView dst, const NoiseSettings& s) {
    if (!dst.valid()) throw InvalidParameterError("procgen::fillNoise: invalid image");
    const uint32 w = dst.width(), h = dst.height();
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            detail::putScalar(dst, x, y,
                detail::sampleNoise(s.type, s.seed,
                                    static_cast<float>(x),
                                    static_cast<float>(y),
                                    s.frequency, s.params));
}

inline void fillNoise(ImageView dst, NoiseType type, uint64 seed,
                      float frequency = 1.0f,
                      const math::noise::FractalParams& params = {}) {
    NoiseSettings s;
    s.type = type; s.seed = seed; s.frequency = frequency; s.params = params;
    fillNoise(dst, s);
}

/// Row-parallel noise fill (SIMD/cache friendly: each row samples its own
/// strip of the lattice). Falls back to the scalar path for tiny images.
inline void fillNoise(ImageView dst, const NoiseSettings& s,
                      const ExecutionPolicy& policy) {
    if (!dst.valid()) throw InvalidParameterError("procgen::fillNoise: invalid image");
    const uint32 w = dst.width(), h = dst.height();
    if (!proc::wantsParallel(policy, static_cast<size_t>(w) * h, 4096)) {
        fillNoise(dst, s);
        return;
    }
    const uint32 width = w;
    const NoiseSettings cfg = s;
    parallelForRows(h, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x)
            detail::putScalar(dst, x, y,
                detail::sampleNoise(cfg.type, cfg.seed,
                                    static_cast<float>(x),
                                    static_cast<float>(y),
                                    cfg.frequency, cfg.params));
    }, policy);
}

} // namespace procgen
} // namespace iml