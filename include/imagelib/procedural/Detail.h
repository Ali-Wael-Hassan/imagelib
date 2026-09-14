#pragma once
// imagelib/procedural/Detail.h
//
// Shared scalar rendering helpers for the procedural subsystem. All renders
// funnel through putScalar / writeRGBA so images of any storage type work.

#include "imagelib/processing/Pixel.h"

namespace iml {
namespace procgen {
namespace detail {

inline float clampf01(float v) noexcept { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

inline float smoothstep(float e0, float e1, float x) noexcept {
    if (e1 <= e0) return x >= e1 ? 1.f : 0.f;
    const float t = clampf01((x - e0) / (e1 - e0));
    return t * t * (3.f - 2.f * t);
}

inline float hsmooth(float x, float a, float b) noexcept {
    if (b <= a) return x >= b ? 1.f : 0.f;
    const float t = clampf01((x - a) / (b - a));
    return t * t * (3.f - 2.f * t);
}

/// Write a scalar into a single destination sample (channel 0 for gray
/// layouts, monochrome value otherwise).
inline void putScalar(ImageView dst, uint32 x, uint32 y, float v) {
    proc::detail::writeRGBA(Pixel(v, v, v, 1.f), dst, static_cast<int32>(x), static_cast<int32>(y));
}

} // namespace detail
} // namespace procgen
} // namespace iml