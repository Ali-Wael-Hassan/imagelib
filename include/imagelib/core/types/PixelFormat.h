#pragma once
#ifndef IMAGELIB_CORE_PIXELFORMAT_H_
#define IMAGELIB_CORE_PIXELFORMAT_H_
/// @file
/// Pixel format: the layout of channels within one pixel.

#include "imagelib/core/types/FixedTypes.h"

namespace iml {

/// Channel layout within one pixel.
enum class PixelFormat : uint8 {
    /// Unknown or unset format.
    Unknown = 0,
    /// 1 channel.
    Gray,
    /// 2 channels.
    GrayAlpha,
    /// 3 channels.
    RGB,
    /// 3 channels, reversed.
    BGR,
    /// 4 channels.
    RGBA,
    /// 4 channels, reversed.
    BGRA,
    /// Arbitrary packed bit layout (not image-plane renderable).
    Packed,
};

/// Returns the canonical PixelFormat for a channel count (1-4).
/// Unknown is returned for zero or unsupported channel counts.
/// @param ch The number of channels.
/// @return The canonical PixelFormat, or Unknown.
constexpr PixelFormat pixelFormatForChannels(uint16 ch) noexcept {
    switch (ch) {
    case 1:
        return PixelFormat::Gray;
    case 2:
        return PixelFormat::GrayAlpha;
    case 3:
        return PixelFormat::RGB;
    case 4:
        return PixelFormat::RGBA;
    default:
        return PixelFormat::Unknown;
    }
}

/// Returns the number of channels for a pixel format.
/// @param f The pixel format to query.
/// @return The channel count, or 0 for Unknown and Packed.
constexpr uint32 pixelFormatChannels(PixelFormat f) noexcept {
    switch (f) {
    case PixelFormat::Gray:
        return 1;
    case PixelFormat::GrayAlpha:
        return 2;
    case PixelFormat::RGB:
    case PixelFormat::BGR:
        return 3;
    case PixelFormat::RGBA:
    case PixelFormat::BGRA:
        return 4;
    case PixelFormat::Unknown:
        return 0;
    case PixelFormat::Packed:
        return 0;
    }
    return 0;
}

} // namespace iml

#endif