#pragma once
/// @file Noise.h
/// Render mathematical noise (iml::math::noise) into images. Every render
/// writes a scalar through iml::pixel::writeRGBA in normalized [0,1], so any
/// storage type (UInt8/16/32/Float32) works.

#include "imagelib/core/image/Image.h"
#include "imagelib/core/image/Pixel.h"
#include "imagelib/math/Scalar.h"
#include "imagelib/math/Noise.h"
#include "imagelib/processing/Primitives.h"

#include <cstdint>

namespace iml {
namespace procgen {

/// Kind of noise field to render.
enum class NoiseType : uint8 {
    /// per-lattice white hash
    White = 0,
    /// interpolated lattice hashes
    Value,
    /// classic gradient lattice
    Perlin,
    /// 2D simplex noise
    Simplex,
    /// F1 cellular distance
    Worley,
    /// F2-F1 cellular "crackle"
    WorleyEdge,
    /// fractal brownian motion
    Fbm,
    /// ridged multifractal
    Ridged,
    /// abs() lobes
    Billow,
    /// hybrid multifractal
    Hybrid,
    /// smudged manifolds
    DomainWarp,
};

/// Parameters controlling a noise render.
struct NoiseSettings {
    /// Noise type to render.
    NoiseType type = NoiseType::Perlin;
    /// Random seed.
    uint64 seed = 0;
    /// Spatial frequency of the base noise.
    float frequency = 1.0f;
    /// Octave/fractal parameters for fractal noise types.
    math::noise::FractalParams params;
};

namespace detail {

/// Write a scalar into a single destination sample (channel 0 for gray
/// layouts, monochrome value otherwise).
inline void putScalar(ImageView dst, uint32 x, uint32 y, float v) {
    pixel::writeRGBA(Pixel(v, v, v, 1.f), dst, static_cast<int32>(x), static_cast<int32>(y));
}

/// Sample a noise field at (x, y) scaled by frequency.
/// @param type Noise type to sample.
/// @param seed Random seed.
/// @param x,y Sample coordinates.
/// @param frequency Spatial frequency multiplier.
/// @param p Fractal/octave parameters for fractal noise types.
/// @return Noise sample in [0,1].
inline float sampleNoise(
    NoiseType type,
    uint64 seed,
    float x,
    float y,
    float frequency,
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
        return math::clamp01(math::noise::worley2D(seed, fx, fy).value());
    case NoiseType::WorleyEdge:
        return math::clamp01(math::noise::worley2D(seed, fx, fy).edge());
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
/// @param dst Destination image.
/// @param s Noise settings.
inline void fillNoise(ImageView dst, const NoiseSettings& s) {
    if (!dst.valid())
        throw InvalidParameterError("procgen::fillNoise: invalid image");
    const uint32 w = dst.width(), h = dst.height();
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            detail::putScalar(
                dst,
                x,
                y,
                detail::sampleNoise(
                    s.type,
                    s.seed,
                    static_cast<float>(x),
                    static_cast<float>(y),
                    s.frequency,
                    s.params));
}

/// Render a noise field of the given type into `dst`.
/// @param dst Destination image.
/// @param type Noise type to render.
/// @param seed Random seed.
/// @param frequency Spatial frequency of the base noise.
/// @param params Octave/fractal parameters for fractal noise types.
inline void fillNoise(
    ImageView dst,
    NoiseType type,
    uint64 seed,
    float frequency = 1.0f,
    const math::noise::FractalParams& params = {}) {
    NoiseSettings s;
    s.type = type;
    s.seed = seed;
    s.frequency = frequency;
    s.params = params;
    fillNoise(dst, s);
}

/// Row-parallel noise fill (SIMD/cache friendly: each row samples its own
/// strip of the lattice). Falls back to the scalar path for tiny images.
/// @param dst Destination image.
/// @param s Noise settings.
/// @param policy Execution policy governing parallelism.
inline void fillNoise(ImageView dst, const NoiseSettings& s, const ExecutionPolicy& policy) {
    if (!dst.valid())
        throw InvalidParameterError("procgen::fillNoise: invalid image");
    const uint32 w = dst.width(), h = dst.height();
    if (!proc::wantsParallel(policy, static_cast<size_t>(w) * h, 4096)) {
        fillNoise(dst, s);
        return;
    }
    const uint32 width = w;
    const NoiseSettings cfg = s;
    parallelForRows(
        h,
        [&](size_t row) {
            const uint32 y = static_cast<uint32>(row);
            for (uint32 x = 0; x < width; ++x)
                detail::putScalar(
                    dst,
                    x,
                    y,
                    detail::sampleNoise(
                        cfg.type,
                        cfg.seed,
                        static_cast<float>(x),
                        static_cast<float>(y),
                        cfg.frequency,
                        cfg.params));
        },
        policy);
}

} // namespace procgen
} // namespace iml