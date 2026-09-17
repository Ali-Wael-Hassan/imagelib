#pragma once
#ifndef IMAGELIB_CORE_PIXELLAYOUT_H_
#define IMAGELIB_CORE_PIXELLAYOUT_H_
/// @file
/// Physical memory layout of pixels: interleaved, planar, or gray.

#include "imagelib/core/types/FixedTypes.h"
#include "imagelib/core/types/PixelFormat.h"

namespace iml {

/// Physical memory layout of pixel samples.
enum class PixelLayout : uint8 {
    /// Layout is not known.
    Unknown = 0,
    /// 1 channel per pixel; contiguous samples, one plane.
    Gray,
    /// N channels per pixel, channel c of pixel i at i*channels + c.
    Interleaved,
    /// One contiguous plane per channel: RRRR... GGGG... BBBB...
    Planar,
};

/// The default memory layout a PixelFormat is stored in. Planar formats are
/// never implied by a pixel format; they are constructed explicitly by the
/// caller (the layout struct carries the plane stride).
/// @param pf The pixel format to map.
/// @return The default layout, or Unknown for unsupported formats.
constexpr PixelLayout layoutForPixelFormat(PixelFormat pf) noexcept {
    switch (pf) {
    case PixelFormat::Gray:
        return PixelLayout::Gray;
    case PixelFormat::GrayAlpha:
    case PixelFormat::RGB:
    case PixelFormat::BGR:
    case PixelFormat::RGBA:
    case PixelFormat::BGRA:
        return PixelLayout::Interleaved;
    default:
        return PixelLayout::Unknown;
    }
}

/// Complete description of how samples of one pixel are laid out in memory.
/// `stride` is in bytes: pixel-pixel for Gray/Interleaved, lane-lane for
/// Planar; `planeStride` (planar only) is the byte offset between channel
/// planes. All zeroed for Unknown.
struct PixelLayoutInfo {
    /// Physical layout kind.
    PixelLayout layout = PixelLayout::Unknown;
    /// Samples per pixel.
    uint16 channels = 0;
    /// Bytes per sample.
    uint32 bytes = 0;
    /// Byte distance between consecutive pixels (Gray/Interleaved) or
    /// consecutive lanes in the same plane (Planar).
    uint32 stride = 0;
    /// Planar: bytes between plane origins.
    uint32 planeStride = 0;

    /// Returns whether the layout describes a usable arrangement.
    /// @return True when layout, channels, bytes, and stride are set.
    constexpr bool valid() const noexcept {
        return layout != PixelLayout::Unknown && channels != 0 && bytes != 0 && stride != 0;
    }

    /// Samples touching a pixel-wide SIMD lane span (for Packed/planar lane
    /// access this is the number of bytes a single pixel's channels occupy in
    /// its plane at one coordinate).
    /// @return The product of channels and bytes.
    constexpr size_t bytesPerPixel() const noexcept {
        return static_cast<size_t>(channels) * bytes;
    }
};

} // namespace iml

#endif