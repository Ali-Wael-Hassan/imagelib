#pragma once
// imagelib/procedural/Masks.h
//
// Analytic masks used for blending, mattes and pattern fills. Every mask is a
// scalar function written into `dst` through writeRGBA (channel 0 for gray
// layouts, monochrome otherwise).

#include "imagelib/core/Image.h"
#include "imagelib/procedural/Detail.h"

#include <cstdint>
#include <cmath>

namespace iml {
namespace procgen {

enum class GradientAxis : uint8 {
    Horizontal = 0,
    Vertical,
    Diagonal, // (x + y) / (w-1 + h-1)
    Radial,   // distance from the image center
};

/// Linear or radial gradient. T is normalized to [0,1] over the chosen axis,
/// then mapped to [a, b].
inline void fillGradient(ImageView dst, GradientAxis axis = GradientAxis::Horizontal,
                         float a = 0.f, float b = 1.f) {
    if (!dst.valid()) throw InvalidParameterError("procgen::fillGradient: invalid image");
    const float w = static_cast<float>(dst.width());
    const float h = static_cast<float>(dst.height());
    const float wm = w - 1.f, hm = h - 1.f;
    const float cx = 0.5f * wm, cy = 0.5f * hm;
    const float maxD = std::sqrt(cx * cx + cy * cy);
    for (uint32 y = 0; y < dst.height(); ++y) {
        for (uint32 x = 0; x < dst.width(); ++x) {
            const float px = static_cast<float>(x), py = static_cast<float>(y);
            float t = 0.f;
            switch (axis) {
                default:
                case GradientAxis::Horizontal: t = wm > 0.f ? px / wm : 0.f; break;
                case GradientAxis::Vertical:   t = hm > 0.f ? py / hm : 0.f; break;
                case GradientAxis::Diagonal:   t = (wm + hm) > 0.f ? (px + py) / (wm + hm) : 0.f; break;
                case GradientAxis::Radial: {
                    const float d = std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
                    t = maxD > 0.f ? d / maxD : 0.f;
                    break;
                }
            }
            const float v = a + (b - a) * detail::clampf01(t);
            detail::putScalar(dst, x, y, v);
        }
    }
}

/// Anti-aliased filled circle, value 1 inside `radius`, fading over `feather`
/// pixels. Center in pixel units.
inline void fillCircleMask(ImageView dst, float cx, float cy, float radius,
                           float feather = 0.f) {
    if (!dst.valid()) throw InvalidParameterError("procgen::fillCircleMask: invalid image");
    if (radius < 0.f) radius = 0.f;
    for (uint32 y = 0; y < dst.height(); ++y) {
        for (uint32 x = 0; x < dst.width(); ++x) {
            const float px = static_cast<float>(x), py = static_cast<float>(y);
            const float d = std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
            detail::putScalar(dst, x, y, 1.f - detail::smoothstep(radius, radius + feather, d));
        }
    }
}

/// Ring band: 1 between `innerRadius` and `outerRadius`, crisp unless the band
/// edges are feathered.
inline void fillRingMask(ImageView dst, float cx, float cy, float innerRadius,
                         float outerRadius, float feather = 0.f) {
    if (!dst.valid()) throw InvalidParameterError("procgen::fillRingMask: invalid image");
    if (innerRadius < 0.f) innerRadius = 0.f;
    if (outerRadius < innerRadius) outerRadius = innerRadius;
    for (uint32 y = 0; y < dst.height(); ++y) {
        for (uint32 x = 0; x < dst.width(); ++x) {
            const float px = static_cast<float>(x), py = static_cast<float>(y);
            const float d = std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
            const float in  = detail::smoothstep(innerRadius - feather * 0.5f,
                                                 innerRadius + feather * 0.5f, d);
            const float out = 1.f - detail::smoothstep(outerRadius - feather * 0.5f,
                                                       outerRadius + feather * 0.5f, d);
            detail::putScalar(dst, x, y, in * out);
        }
    }
}

/// Checkerboard of `tilesX` by `tilesY` cells ("white" cells get `b`, black `a`).
inline void fillCheckerboard(ImageView dst, uint32 tilesX, uint32 tilesY,
                             float a = 0.f, float b = 1.f) {
    if (!dst.valid()) throw InvalidParameterError("procgen::fillCheckerboard: invalid image");
    if (tilesX == 0) tilesX = 1;
    if (tilesY == 0) tilesY = 1;
    const float w = static_cast<float>(dst.width());
    const float h = static_cast<float>(dst.height());
    for (uint32 y = 0; y < dst.height(); ++y) {
        for (uint32 x = 0; x < dst.width(); ++x) {
            const uint32 cx = static_cast<uint32>((static_cast<float>(x) * tilesX) / w);
            const uint32 cy = static_cast<uint32>((static_cast<float>(y) * tilesY) / h);
            detail::putScalar(dst, x, y, ((cx + cy) & 1u) == 0 ? a : b);
        }
    }
}

} // namespace procgen
} // namespace iml