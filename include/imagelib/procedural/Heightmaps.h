#pragma once
// imagelib/procedural/Heightmaps.h
//
// Height map utilities: normals, hillshade lighting and contour overlay. The
// input is a single-channel height field read through readNorm ([0,1]); the
// output is an image (RGB for normal maps, scalar shading otherwise).

#include "imagelib/core/Image.h"
#include "imagelib/math/Vector.h"
#include "imagelib/procedural/Detail.h"

#include <cmath>
#include <cstdint>

namespace iml {
namespace procgen {

namespace detail {

inline uint32 clampIndex(int32 i, uint32 n) noexcept {
    return static_cast<uint32>(i < 0 ? 0 : (i >= static_cast<int32>(n) ? n - 1 : i));
}

/// Central-difference gradient of the height field at (x, y) with clamp borders.
inline void heightGradient(const ConstImageView& hgt, uint32 x, uint32 y,
                           float& gx, float& gy) {
    const int32 w = static_cast<int32>(hgt.width());
    const int32 h = static_cast<int32>(hgt.height());
    const uint32 xm = clampIndex(static_cast<int32>(x) - 1, hgt.width());
    const uint32 xp = clampIndex(static_cast<int32>(x) + 1, hgt.width());
    const uint32 ym = clampIndex(static_cast<int32>(y) - 1, hgt.height());
    const uint32 yp = clampIndex(static_cast<int32>(y) + 1, hgt.height());
    (void)w; (void)h;
    gx = proc::detail::readNorm(hgt, xp, y, 0) - proc::detail::readNorm(hgt, xm, y, 0);
    gy = proc::detail::readNorm(hgt, x, yp, 0) - proc::detail::readNorm(hgt, x, ym, 0);
}

} // namespace detail

/// Surface normal map from a height field. `strength` scales the horizontal
/// gradient before normalizing (larger = more dramatic relief). The output is
/// RGB: (nx, ny, nz) each remapped from [-1,1] to [0,1].
inline void heightmapNormal(const ConstImageView& height, ImageView dst,
                            float strength = 1.f) {
    if (!height.valid() || !dst.valid())
        throw InvalidParameterError("procgen::heightmapNormal: invalid view");
    if (height.width() != dst.width() || height.height() != dst.height())
        throw InvalidParameterError("procgen::heightmapNormal: dimension mismatch");
    for (uint32 y = 0; y < height.height(); ++y) {
        for (uint32 x = 0; x < height.width(); ++x) {
            float gx = 0.f, gy = 0.f;
            detail::heightGradient(height, x, y, gx, gy);
            const float sx = -gx * strength, sy = -gy * strength;
            const float inv = 1.f / std::sqrt(sx * sx + sy * sy + 1.f);
            const float nx = sx * inv, ny = sy * inv, nz = inv;
            proc::detail::writeRGBA(Pixel((nx + 1.f) * 0.5f, (ny + 1.f) * 0.5f,
                                          (nz + 1.f) * 0.5f, 1.f),
                                    dst, static_cast<int32>(x), static_cast<int32>(y));
        }
    }
}

/// Diffuse hillshade: light from `azimuthDeg` (degrees clockwise from north)
/// and `elevationDeg` above horizon, plus ambient. Writes scalar shading into
/// `dst`.
inline void heightmapShade(const ConstImageView& height, ImageView dst,
                           float azimuthDeg = 45.f, float elevationDeg = 30.f,
                           float ambient = 0.35f) {
    if (!height.valid() || !dst.valid())
        throw InvalidParameterError("procgen::heightmapShade: invalid view");
    if (height.width() != dst.width() || height.height() != dst.height())
        throw InvalidParameterError("procgen::heightmapShade: dimension mismatch");
    const float az = azimuthDeg * 3.14159265358979323846f / 180.f;
    const float el = elevationDeg * 3.14159265358979323846f / 180.f;
const math::Vec3 light(-std::sin(az) * std::cos(el), -std::cos(az) * std::cos(el),
                      std::sin(el));
    ambient = detail::clampf01(ambient);
    for (uint32 y = 0; y < height.height(); ++y) {
        for (uint32 x = 0; x < height.width(); ++x) {
            float gx = 0.f, gy = 0.f;
            detail::heightGradient(height, x, y, gx, gy);
            math::Vec3 n(-gx, -gy, 1.f);
            n.normalize();
            const float diff = std::max(0.f, n.dot(light));
            detail::putScalar(dst, x, y, ambient + (1.f - ambient) * diff);
        }
    }
}

/// Contour overlay on the height field itself: heights are preserved, every
/// `step` (normalized) a dark line of half-width `lineWidth` is drawn.
inline void heightmapContours(const ConstImageView& height, ImageView dst,
                              float step = 1.f / 16.f, float lineWidth = 0.03f) {
    if (!height.valid() || !dst.valid())
        throw InvalidParameterError("procgen::heightmapContours: invalid view");
    if (height.width() != dst.width() || height.height() != dst.height())
        throw InvalidParameterError("procgen::heightmapContours: dimension mismatch");
    if (step <= 0.f || lineWidth <= 0.f)
        throw InvalidParameterError("procgen::heightmapContours: non-positive step/width");
    const float yl = lineWidth * step; // distance threshold in height units
    for (uint32 y = 0; y < height.height(); ++y) {
        for (uint32 x = 0; x < height.width(); ++x) {
            const float h = proc::detail::readNorm(height, x, y, 0);
            const float level = h / step;
            const float frac = level - std::floor(level);
            const float dist = std::min(frac, 1.f - frac) * step;
            detail::putScalar(dst, x, y, dist < yl ? 0.f : h);
        }
    }
}

} // namespace procgen
} // namespace iml