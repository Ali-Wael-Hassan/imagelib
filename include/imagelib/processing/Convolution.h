#pragma once
/// @file Convolution.h
/// The Kernel container applies a small 2D weight matrix to an image, producing
/// a raw typed value, a normalized [0,1] float, or a canonical Pixel. Uses
/// correlation orientation (no kernel flip), the image processing convention.

#include "imagelib/core/image/Pixel.h"
#include "imagelib/core/image/Image.h"

#include <cstdint>
#include <vector>

namespace iml {
namespace proc {
namespace conv {

/// How out-of-bounds samples are produced during a filter.
enum class BorderMode : uint8 { Clamp = 0, Mirror, Wrap, Zero };

namespace detail {

/// Clamp a coordinate into [0, n).
constexpr int32 clampCoord(int32 x, int32 n) noexcept { return x < 0 ? 0 : (x >= n ? n - 1 : x); }

/// Mirror a coordinate into [0, n): -1 -> 1, n -> n - 1, -2 -> 2, ...
constexpr int32 reflectIndex(int32 i, int32 n) noexcept {
    while (i < 0 || i >= n) {
        if (i < 0)
            i = -i - 1;
        else
            i = 2 * n - i - 1;
    }
    return i;
}

/// Maps an out-of-bounds sample coordinate to an in-bounds one. When border is
/// Zero and the coordinate is outside, returns false (sample is 0).
inline bool mapCoordinate(
    int32 sx,
    int32 sy,
    int32 w,
    int32 h,
    BorderMode border,
    int32& outX,
    int32& outY) noexcept {
    switch (border) {
    case BorderMode::Clamp:
        outX = clampCoord(sx, w);
        outY = clampCoord(sy, h);
        return true;
    case BorderMode::Mirror:
        outX = reflectIndex(sx, w);
        outY = reflectIndex(sy, h);
        return true;
    case BorderMode::Wrap:
        outX = ((sx % w) + w) % w;
        outY = ((sy % h) + h) % h;
        return true;
    case BorderMode::Zero:
    default:
        if (sx < 0 || sx >= w || sy < 0 || sy >= h)
            return false;
        outX = sx;
        outY = sy;
        return true;
    }
}

/// Normalized border sample.
inline float
sampleNorm(const ConstImageView& img, int32 x, int32 y, uint32 c, BorderMode border) noexcept {
    int32 sx = 0, sy = 0;
    if (!mapCoordinate(
            x,
            y,
            static_cast<int32>(img.width()),
            static_cast<int32>(img.height()),
            border,
            sx,
            sy))
        return 0.f;
    return pixel::readNorm(img, sx, sy, c);
}

/// Raw typed border sample (uses the storage type T directly).
template <typename T>
inline T
sampleRaw(const ConstImageView& img, int32 x, int32 y, uint32 c, BorderMode border) noexcept {
    int32 sx = 0, sy = 0;
    if (!mapCoordinate(
            x,
            y,
            static_cast<int32>(img.width()),
            static_cast<int32>(img.height()),
            border,
            sx,
            sy))
        return T(0);
    return img.getSample<T>(sx, sy, c);
}

} // namespace detail

/// A small 2D convolution kernel (odd dimensions; center = weight origin).
struct Kernel {
    /// Kernel width (odd).
    int width = 0;
    /// Kernel height (odd).
    int height = 0;
    /// Weight matrix, row-major, width*height entries.
    std::vector<float> data;

    /// Default-constructed kernel (invalid until populated).
    Kernel() noexcept = default;
    /// Construct from dimensions and row-major weight data.
    Kernel(int w, int h, std::vector<float> d);

    /// True when dimensions are positive odd numbers and data matches.
    bool valid() const noexcept;
    /// Weight at local offset (kx, ky); (0,0) is the kernel center.
    float at(int kx, int ky) const noexcept;
    /// X radius (width / 2).
    int radiusX() const noexcept;
    /// Y radius (height / 2).
    int radiusY() const noexcept;

    /// Applies the kernel to the image at (x, y, c) using normalized [0, 1]
    /// samples. The result is the weighted sum in that representation.
    /// @param img Source image.
    /// @param x,y,c Sample coordinate and channel.
    /// @param border Out-of-bounds border mode.
    /// @return Weighted sum of normalized samples.
    float applyNorm(const ConstImageView& img, int32 x, int32 y, uint32 c, BorderMode border)
        const noexcept;

    /// Applies the kernel to the image at (x, y) reading canonical Pixels
    /// (layout-aware channel order). The result is a canonical Pixel.
    /// @param img Source image.
    /// @param x,y Sample coordinate.
    /// @param border Out-of-bounds border mode.
    /// @return Weighted-sum canonical Pixel.
    Pixel applyPixel(const ConstImageView& img, int32 x, int32 y, BorderMode border) const noexcept;

    /// Applies the kernel to the image at (x, y, c) reading raw samples of the
    /// value representation T (UInt8/UInt16/UInt32/Float32).
    /// @tparam T Value representation of the image samples.
    /// @param img Source image.
    /// @param x,y,c Sample coordinate and channel.
    /// @param border Out-of-bounds border mode.
    /// @return Weighted sum cast back to T.
    template <typename T>
    T apply(const ConstImageView& img, int32 x, int32 y, uint32 c, BorderMode border)
        const noexcept {
        const int rx = radiusX();
        const int ry = radiusY();
        float acc = 0.f;
        for (int ky = -ry; ky <= ry; ++ky)
            for (int kx = -rx; kx <= rx; ++kx) {
                const float wgt = at(kx + rx, ky + ry);
                if (wgt != 0.f)
                    acc += wgt *
                           static_cast<float>(detail::sampleRaw<T>(img, x + kx, y + ky, c, border));
            }
        return static_cast<T>(acc);
    }
};

} // namespace conv
} // namespace proc
} // namespace iml