#ifndef IMAGELIB_CORE_PIXEL_H_
#define IMAGELIB_CORE_PIXEL_H_

/// @file Pixel.h
/// Pixel value types and normalized [0, 1] sample I/O at the image layer.

#include "imagelib/core/image/ImageView.h"
#include "imagelib/math/Scalar.h"

#include <algorithm>
#include <cmath>

namespace iml {

/// Floating-point pixel (channels always treated as RGBA; unused channels are
/// filled when reading from narrower layouts).
struct Pixel {
    /// Canonical RGBA channel values in the normalized [0, 1] range.
    float r = 0.f, g = 0.f, b = 0.f, a = 1.f;

    /// Constructs a pixel with default (transparent black) values.
    constexpr Pixel() noexcept = default;
    /// Constructs a pixel from RGBA channel values.
    /// @param rr Red channel.
    /// @param gg Green channel.
    /// @param bb Blue channel.
    /// @param aa Alpha channel; defaults to 1.
    constexpr Pixel(float rr, float gg, float bb, float aa = 1.f) noexcept
        : r(rr), g(gg), b(bb), a(aa) {}

    /// Adds two pixels component-wise.
    /// @param o The other pixel.
    /// @return Result of the addition.
    constexpr Pixel operator+(const Pixel& o) const noexcept {
        return {r + o.r, g + o.g, b + o.b, a + o.a};
    }
    /// Subtracts two pixels component-wise.
    /// @param o The other pixel.
    /// @return Result of the subtraction.
    constexpr Pixel operator-(const Pixel& o) const noexcept {
        return {r - o.r, g - o.g, b - o.b, a - o.a};
    }
    /// Multiplies two pixels component-wise.
    /// @param o The other pixel.
    /// @return Result of the multiplication.
    constexpr Pixel operator*(const Pixel& o) const noexcept {
        return {r * o.r, g * o.g, b * o.b, a * o.a};
    }
    /// Scales all channels by a scalar.
    /// @param s The scalar multiplier.
    /// @return Result of the scaling.
    constexpr Pixel operator*(float s) const noexcept { return {r * s, g * s, b * s, a * s}; }
    /// Scales all channels by a scalar (scalar on the left).
    /// @param s The scalar multiplier.
    /// @param p The pixel.
    /// @return Result of the scaling.
    friend constexpr Pixel operator*(float s, const Pixel& p) noexcept { return p * s; }
    /// Divides all channels by a scalar.
    /// @param s The scalar divisor.
    /// @return Result of the division.
    constexpr Pixel operator/(float s) const noexcept { return {r / s, g / s, b / s, a / s}; }
    /// Divides two pixels component-wise.
    /// @param o The other pixel.
    /// @return Result of the division.
    constexpr Pixel operator/(const Pixel& o) const noexcept {
        return {r / o.r, g / o.g, b / o.b, a / o.a};
    }

    /// Adds another pixel component-wise in place.
    /// @param o The other pixel.
    /// @return Reference to this pixel.
    Pixel& operator+=(const Pixel& o) noexcept;
    /// Subtracts another pixel component-wise in place.
    /// @param o The other pixel.
    /// @return Reference to this pixel.
    Pixel& operator-=(const Pixel& o) noexcept;
    /// Multiplies another pixel component-wise in place.
    /// @param o The other pixel.
    /// @return Reference to this pixel.
    Pixel& operator*=(const Pixel& o) noexcept;
    /// Scales all channels in place.
    /// @param s The scalar multiplier.
    /// @return Reference to this pixel.
    Pixel& operator*=(float s) noexcept;
    /// Divides all channels in place.
    /// @param s The scalar divisor.
    /// @return Reference to this pixel.
    Pixel& operator/=(float s) noexcept;

    /// Weighted luminance of the RGB channels (alpha ignored).
    /// @return Luminance in [0, 1].
    float luminance() const noexcept;

    /// Returns true when all channels are equal.
    /// @param o The other pixel.
    /// @return True when equal.
    constexpr bool operator==(const Pixel& o) const noexcept {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
    /// Returns true when any channel differs.
    /// @param o The other pixel.
    /// @return True when not equal.
    constexpr bool operator!=(const Pixel& o) const noexcept { return !(*this == o); }
};

/// Packed 8-bit pixel (one byte per RGBA channel).
struct Pixel8u {
    /// Channel bytes; a defaults to opaque.
    uint8 r, g, b, a;
    /// Constructs an opaque black pixel.
    constexpr Pixel8u() noexcept : r(0), g(0), b(0), a(255) {}
    /// Constructs a pixel from channel bytes.
    /// @param rr Red channel.
    /// @param gg Green channel.
    /// @param bb Blue channel.
    /// @param aa Alpha channel; defaults to opaque.
    constexpr Pixel8u(uint8 rr, uint8 gg, uint8 bb, uint8 aa = 255) noexcept
        : r(rr), g(gg), b(bb), a(aa) {}
    /// Converts a normalized Pixel to 8-bit, clamping to [0, 1].
    /// @param p The normalized pixel.
    explicit constexpr Pixel8u(const Pixel& p) noexcept
        : r(static_cast<uint8>(std::min(1.f, p.r) * 255.f + 0.5f)),
          g(static_cast<uint8>(std::min(1.f, p.g) * 255.f + 0.5f)),
          b(static_cast<uint8>(std::min(1.f, p.b) * 255.f + 0.5f)),
          a(static_cast<uint8>(std::min(1.f, p.a) * 255.f + 0.5f)) {}
    /// Converts to a normalized Pixel in [0, 1].
    /// @return The normalized pixel.
    explicit constexpr operator Pixel() const noexcept {
        return {r * 1.f / 255.f, g * 1.f / 255.f, b * 1.f / 255.f, a * 1.f / 255.f};
    }
};

namespace pixel {

/// Read a sample as a normalized [0, 1] float.
/// @param src The source image view.
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @return The normalized sample value.
/// @throws InvalidParameterError when out of bounds.
/// @throws UnsupportedFormatError on unsupported data types.
inline float readNorm(const ConstImageView& src, int32 x, int32 y, uint32 c) {
    switch (src.dataType()) {
    case DataType::UInt8:
        return src.getSample<uint8>(x, y, c) * (1.f / 255.f);
    case DataType::UInt16:
        return src.getSample<uint16>(x, y, c) * (1.f / 65535.f);
    case DataType::UInt32:
        return src.getSample<uint32>(x, y, c) * (1.f / 4294967295.f);
    case DataType::Float32:
        return src.getSample<float>(x, y, c);
    default:
        throw UnsupportedFormatError("pixel::readNorm: unsupported source type");
    }
}

/// Convert a normalized [0, 1] sample to the image's storage type and store it.
/// @param dst The destination image view.
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @param v The normalized value to store.
/// @throws InvalidParameterError when out of bounds.
/// @throws UnsupportedFormatError on unsupported data types.
inline void writeNorm(ImageView dst, int32 x, int32 y, uint32 c, float v) {
    switch (dst.dataType()) {
    case DataType::UInt8:
        dst.setSample<uint8>(x, y, c, static_cast<uint8>(math::clamp01(v) * 255.f + 0.5f));
        break;
    case DataType::UInt16:
        dst.setSample<uint16>(x, y, c, static_cast<uint16>(math::clamp01(v) * 65535.f + 0.5f));
        break;
    case DataType::UInt32: {
        const double q = math::clamp01(v) * 4294967295.0 + 0.5;
        dst.setSample<uint32>(x, y, c, static_cast<uint32>(q));
        break;
    }
    case DataType::Float32:
        dst.setSample<float>(x, y, c, v);
        break;
    default:
        throw UnsupportedFormatError("pixel::writeNorm: unsupported destination type");
    }
}

/// Layout-aware read: maps Gray/GrayAlpha/RGB/BGR/RGBA/BGRA into a canonical
/// RGBA Pixel with correct channel order.
/// @param src The source image view.
/// @param x Column index.
/// @param y Row index.
/// @return The canonical RGBA Pixel.
/// @throws InvalidParameterError when out of bounds.
/// @throws UnsupportedFormatError on unsupported pixel formats.
inline Pixel readPixel(const ConstImageView& src, int32 x, int32 y) {
    switch (src.pixelFormat()) {
    case PixelFormat::Gray:
        return Pixel(
            pixel::readNorm(src, x, y, 0),
            pixel::readNorm(src, x, y, 0),
            pixel::readNorm(src, x, y, 0),
            1.f);
    case PixelFormat::GrayAlpha:
        return Pixel(
            pixel::readNorm(src, x, y, 0),
            pixel::readNorm(src, x, y, 0),
            pixel::readNorm(src, x, y, 0),
            pixel::readNorm(src, x, y, 1));
    case PixelFormat::RGB:
        return Pixel(
            pixel::readNorm(src, x, y, 0),
            pixel::readNorm(src, x, y, 1),
            pixel::readNorm(src, x, y, 2),
            1.f);
    case PixelFormat::BGR:
        return Pixel(
            pixel::readNorm(src, x, y, 2),
            pixel::readNorm(src, x, y, 1),
            pixel::readNorm(src, x, y, 0),
            1.f);
    case PixelFormat::RGBA:
        return Pixel(
            pixel::readNorm(src, x, y, 0),
            pixel::readNorm(src, x, y, 1),
            pixel::readNorm(src, x, y, 2),
            pixel::readNorm(src, x, y, 3));
    case PixelFormat::BGRA:
        return Pixel(
            pixel::readNorm(src, x, y, 2),
            pixel::readNorm(src, x, y, 1),
            pixel::readNorm(src, x, y, 0),
            pixel::readNorm(src, x, y, 3));
    default:
        break;
    }
    throw UnsupportedFormatError("pixel::readPixel: unsupported pixel format");
}

/// Canonical-RGBA -> layout-aware write. Writes only the channels the dst
/// actually has; keeps canonical order (RGB*), reversing for BGR* layouts.
/// @param p The canonical RGBA Pixel.
/// @param dst The destination image view.
/// @param x Column index.
/// @param y Row index.
/// @throws InvalidParameterError when out of bounds.
/// @throws UnsupportedFormatError on unsupported pixel formats.
inline void writeRGBA(const Pixel& p, ImageView dst, int32 x, int32 y) {
    switch (dst.pixelFormat()) {
    case PixelFormat::Gray:
    case PixelFormat::GrayAlpha:
        pixel::writeNorm(dst, x, y, 0, p.r);
        break;
    case PixelFormat::RGB:
        pixel::writeNorm(dst, x, y, 0, p.r);
        pixel::writeNorm(dst, x, y, 1, p.g);
        pixel::writeNorm(dst, x, y, 2, p.b);
        break;
    case PixelFormat::BGR:
        pixel::writeNorm(dst, x, y, 0, p.b);
        pixel::writeNorm(dst, x, y, 1, p.g);
        pixel::writeNorm(dst, x, y, 2, p.r);
        break;
    case PixelFormat::RGBA:
        pixel::writeNorm(dst, x, y, 0, p.r);
        pixel::writeNorm(dst, x, y, 1, p.g);
        pixel::writeNorm(dst, x, y, 2, p.b);
        pixel::writeNorm(dst, x, y, 3, p.a);
        break;
    case PixelFormat::BGRA:
        pixel::writeNorm(dst, x, y, 0, p.b);
        pixel::writeNorm(dst, x, y, 1, p.g);
        pixel::writeNorm(dst, x, y, 2, p.r);
        pixel::writeNorm(dst, x, y, 3, p.a);
        break;
    default:
        throw UnsupportedFormatError("pixel::writeRGBA: unsupported destination format");
    }
}

} // namespace pixel
} // namespace iml

#endif