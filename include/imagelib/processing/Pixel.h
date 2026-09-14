#pragma once
#define IMAGELIB_PROCESSING_PIXEL_H_
// imagelib/processing/Pixel.h
//
// Pixel value types and normalized sample I/O used by every processing
// operation. All image ops in ImageLib process normalized samples in
// [0, 1]; readNorm/writeNorm convert to and from the image's storage type
// (UInt8/UInt16/UInt32/Float32), so kernels are identical across formats.

#include "imagelib/core/ImageView.h"
#include "imagelib/core/ExecutionPolicy.h"
#include "imagelib/threading/ParallelFor.h"

#include <algorithm>
#include <cmath>

namespace iml {

/// Floating-point pixel (channels always treated as RGBA; unused channels are
/// filled when reading from narrower layouts).
struct Pixel {
    float r = 0.f, g = 0.f, b = 0.f, a = 1.f;

    constexpr Pixel() noexcept = default;
    constexpr Pixel(float rr, float gg, float bb, float aa = 1.f) noexcept
        : r(rr), g(gg), b(bb), a(aa) {}

    constexpr Pixel operator+(const Pixel& o) const noexcept { return {r + o.r, g + o.g, b + o.b, a + o.a}; }
    constexpr Pixel operator-(const Pixel& o) const noexcept { return {r - o.r, g - o.g, b - o.b, a - o.a}; }
    constexpr Pixel operator*(const Pixel& o) const noexcept { return {r * o.r, g * o.g, b * o.b, a * o.a}; }
    constexpr Pixel operator*(float s) const noexcept { return {r * s, g * s, b * s, a * s}; }
    friend constexpr Pixel operator*(float s, const Pixel& p) noexcept { return p * s; }
    constexpr Pixel operator/(float s) const noexcept { return {r / s, g / s, b / s, a / s}; }
    constexpr Pixel operator/(const Pixel& o) const noexcept { return {r / o.r, g / o.g, b / o.b, a / o.a}; }

    Pixel& operator+=(const Pixel& o) noexcept;
    Pixel& operator-=(const Pixel& o) noexcept;
    Pixel& operator*=(const Pixel& o) noexcept;
    Pixel& operator*=(float s) noexcept;
    Pixel& operator/=(float s) noexcept;

    /// Weighted luminance of the RGB channels (alpha ignored).
    float luminance() const noexcept;

    constexpr bool operator==(const Pixel& o) const noexcept {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
    constexpr bool operator!=(const Pixel& o) const noexcept { return !(*this == o); }
};

/// Packed 8-bit pixel (one byte per RGBA channel).
struct Pixel8u {
    uint8 r, g, b, a;
    constexpr Pixel8u() noexcept : r(0), g(0), b(0), a(255) {}
    constexpr Pixel8u(uint8 rr, uint8 gg, uint8 bb, uint8 aa = 255) noexcept
        : r(rr), g(gg), b(bb), a(aa) {}
    explicit constexpr Pixel8u(const Pixel& p) noexcept
        : r(static_cast<uint8>(std::min(1.f, p.r) * 255.f + 0.5f)),
          g(static_cast<uint8>(std::min(1.f, p.g) * 255.f + 0.5f)),
          b(static_cast<uint8>(std::min(1.f, p.b) * 255.f + 0.5f)),
          a(static_cast<uint8>(std::min(1.f, p.a) * 255.f + 0.5f)) {}
    explicit constexpr operator Pixel() const noexcept { return {r * 1.f / 255.f, g * 1.f / 255.f, b * 1.f / 255.f, a * 1.f / 255.f}; }
};

Pixel clamp01(const Pixel& p) noexcept;

Pixel lerp(const Pixel& a, const Pixel& b, float t) noexcept;

namespace proc {
// Expose the threading helper so processing kernels can gate on the policy.
using ::iml::detail::wantsParallel;

namespace detail {

/// Clamp to [0,1].
float c01(float v) noexcept;

/// Clamp to [min,max].
float clampf(float v, float lo, float hi) noexcept;

/// Convert a normalized [0,1] sample to the image's storage type.
void writeNorm(ImageView dst, int32 x, int32 y, uint32 c, float v);

/// Read a sample as a normalized [0,1] float.
float readNorm(const ConstImageView& src, int32 x, int32 y, uint32 c);

/// Pixel layout-aware read: maps any layout (Gray/GrayAlpha/RGB/BGR/RGBA/BGRA)
/// into a canonical RGBA Pixel with correct channel order.
Pixel readPixel(const ConstImageView& src, int32 x, int32 y);

/// Canonical-RGBA -> RGB element writes. Writes only the channels the dst
/// actually has; keeps canonical order (RGB*), reversing for BGR* layouts.
void writeRGBA(const Pixel& p, ImageView dst, int32 x, int32 y);

} // namespace detail
} // namespace proc
} // namespace iml

#include "imagelib/processing/Pixel.tpp"