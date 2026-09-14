#pragma once
// imagelib/procedural/Textures.h
//
// Classic procedural color textures: wood, marble, clouds and plasma. Each is
// generated analytically from per-pixel math plus iml noise; the palette is
// written through writeRGBA so the storage type is flexible.

#include "imagelib/core/Image.h"
#include "imagelib/math/Noise.h"
#include "imagelib/processing/Color.h"
#include "imagelib/procedural/Detail.h"

#include <cmath>
#include <cstdint>

namespace iml {
namespace procgen {

/// Concentric wood rings with turbulent distortion. `frequency` sets ring
/// count across the image diagonal, `turbulence` the ring wobble amount.
inline void fillWood(ImageView dst, uint64 seed, float frequency = 6.f,
                     float turbulence = 2.f) {
    if (!dst.valid()) throw InvalidParameterError("procgen::fillWood: invalid image");
    const float w = static_cast<float>(dst.width());
    const float h = static_cast<float>(dst.height());
    for (uint32 y = 0; y < dst.height(); ++y) {
        for (uint32 x = 0; x < dst.width(); ++x) {
            const float px = static_cast<float>(x);
            const float py = static_cast<float>(y);
            const float dx = (px - 0.5f * w) / w;
            const float dy = (py - 0.5f * h) / w;
            const float d = std::sqrt(dx * dx + dy * dy);
            const float wobble = (math::noise::gradientNoise2D(seed, px * 0.06f, py * 0.06f)
                                  * 2.f - 1.f) * turbulence;
            const float ring = 0.5f + 0.5f * std::sin(d * 6.28318530717958647692f * frequency + wobble);
            const float grain = math::noise::valueNoise2D(seed ^ 0x1234ABCDULL,
                                                          px * 0.5f, py * 0.5f);
            Pixel dark(0.22f, 0.12f, 0.06f, 1.f);
            Pixel light(0.62f, 0.44f, 0.26f, 1.f);
            Pixel c = lerp(dark, light, std::pow(ring, 0.6f));
            const float g = (grain - 0.5f) * 0.08f;
            proc::detail::writeRGBA(Pixel(detail::clampf01(c.r + g),
                                          detail::clampf01(c.g + g),
                                          detail::clampf01(c.b + g), 1.f),
                                    dst, static_cast<int32>(x), static_cast<int32>(y));
        }
    }
}

/// Light gray marble with turbulence-smeared veins. `frequency` scales the
/// vein pattern, `turbulence` the displacement strength.
inline void fillMarble(ImageView dst, uint64 seed, float frequency = 4.f,
                       float turbulence = 2.f) {
    if (!dst.valid()) throw InvalidParameterError("procgen::fillMarble: invalid image");
    math::noise::FractalParams fp;
    fp.octaves = 4;
    for (uint32 y = 0; y < dst.height(); ++y) {
        for (uint32 x = 0; x < dst.width(); ++x) {
            const float px = static_cast<float>(x);
            const float py = static_cast<float>(y);
            const float warp = math::noise::fbm2D(seed, px * 0.02f, py * 0.02f, fp)
                               * turbulence;
            const float q = px * frequency * 0.01f + warp;
            const float v = 0.5f + 0.5f * std::sin(q * 3.14159265358979323846f);
            // veins sit at the bright crests
            const float vein = detail::hsmooth(v, 0.72f, 0.9f);
            const float gray = 0.25f + 0.55f * detail::clampf01(v);
            Pixel c(gray, gray, gray + 0.08f * vein, 1.f);
            proc::detail::writeRGBA(Pixel(detail::clampf01(c.r),
                                          detail::clampf01(c.g),
                                          detail::clampf01(c.b), 1.f),
                                    dst, static_cast<int32>(x), static_cast<int32>(y));
        }
    }
}

/// Soft white clouds over a blue sky, using fractal FBM noise.
inline void fillClouds(ImageView dst, uint64 seed, float frequency = 0.02f,
                       int octaves = 4) {
    if (!dst.valid()) throw InvalidParameterError("procgen::fillClouds: invalid image");
    if (octaves < 1) octaves = 1;
    math::noise::FractalParams fp;
    fp.octaves = octaves;
    for (uint32 y = 0; y < dst.height(); ++y) {
        for (uint32 x = 0; x < dst.width(); ++x) {
            const float px = static_cast<float>(x);
            const float py = static_cast<float>(y);
            const float n = math::noise::fbm2D(seed, px * frequency, py * frequency, fp)
                            * 0.5f + 0.5f;
            const float cloud = detail::hsmooth(n, 0.40f, 0.78f);
            Pixel sky(0.40f, 0.60f, 0.90f, 1.f);
            Pixel white(1.f, 1.f, 1.f, 1.f);
            Pixel c = lerp(sky, white, cloud);
            proc::detail::writeRGBA(c, dst, static_cast<int32>(x), static_cast<int32>(y));
        }
    }
}

/// Colorful "plasma" in HSV space: hue from FBM, value boosted by detail noise.
inline void fillPlasma(ImageView dst, uint64 seed, float frequency = 3.f,
                       int octaves = 2) {
    if (!dst.valid()) throw InvalidParameterError("procgen::fillPlasma: invalid image");
    if (octaves < 1) octaves = 1;
    math::noise::FractalParams fp;
    fp.octaves = octaves;
    for (uint32 y = 0; y < dst.height(); ++y) {
        for (uint32 x = 0; x < dst.width(); ++x) {
            const float px = static_cast<float>(x);
            const float py = static_cast<float>(y);
            const float hue = math::noise::fbm2D(seed, px * frequency, py * frequency, fp)
                              * 0.5f + 0.5f;
            const float val = 0.55f + 0.45f * math::noise::valueNoise2D(
                                         seed ^ 0xDEADBEEFULL, px * 0.25f, py * 0.25f);
            Pixel c = proc::color::hsvToRgb(Pixel(hue, 0.85f, val, 1.f));
            proc::detail::writeRGBA(c, dst, static_cast<int32>(x), static_cast<int32>(y));
        }
    }
}

} // namespace procgen
} // namespace iml