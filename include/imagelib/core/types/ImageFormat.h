#pragma once
#ifndef IMAGELIB_CORE_IMAGEFORMAT_H_
#define IMAGELIB_CORE_IMAGEFORMAT_H_
/// @file
/// Image format: a pixel format + data type pairing.

#include "imagelib/core/types/FixedTypes.h"
#include "imagelib/core/types/PixelFormat.h"
#include "imagelib/core/types/DataTypes.h"

#include <cstddef>

namespace iml {

/// Pairing of a pixel format with a scalar data type.
struct ImageFormat {
    /// Channel layout within each pixel.
    PixelFormat pixelFormat = PixelFormat::Unknown;
    /// Scalar type of each sample.
    DataType dataType = DataType::Unknown;

    /// Default-constructs an unknown format.
    constexpr ImageFormat() noexcept = default;
    /// Constructs a format from a pixel format and data type.
    /// @param pf The channel layout.
    /// @param dt The scalar data type.
    constexpr ImageFormat(PixelFormat pf, DataType dt) noexcept : pixelFormat(pf), dataType(dt) {}

    /// Returns whether both parts are set to a known value.
    /// @return True when the format is usable.
    constexpr bool valid() const noexcept {
        return pixelFormat != PixelFormat::Unknown && dataType != DataType::Unknown;
    }
    /// Compares two formats for equality.
    /// @param o The other format.
    /// @return True when both parts match.
    constexpr bool operator==(const ImageFormat& o) const noexcept {
        return pixelFormat == o.pixelFormat && dataType == o.dataType;
    }
    /// Compares two formats for inequality.
    /// @param o The other format.
    /// @return True when the formats differ.
    constexpr bool operator!=(const ImageFormat& o) const noexcept { return !(*this == o); }
    /// Number of channels implied by the pixel format.
    /// @return The channel count of the pixel format.
    constexpr uint32 channels() const noexcept { return pixelFormatChannels(pixelFormat); }
    /// Bytes per sample.
    /// @return The byte size of one scalar sample.
    constexpr size_t sampleSize() const noexcept { return dataTypeSize(dataType); }
    /// Bytes per pixel (bytes-per-channel * channels).
    /// @return The product of sample size and channel count.
    constexpr size_t pixelSize() const noexcept { return sampleSize() * channels(); }
};

/// Common image format presets.
namespace fmt {
/// 8-bit grayscale.
constexpr ImageFormat gray8 = ImageFormat(PixelFormat::Gray, DataType::UInt8);
/// 16-bit grayscale.
constexpr ImageFormat gray16 = ImageFormat(PixelFormat::Gray, DataType::UInt16);
/// 32-bit float grayscale.
constexpr ImageFormat gray32f = ImageFormat(PixelFormat::Gray, DataType::Float32);
/// 8-bit RGB.
constexpr ImageFormat rgb8 = ImageFormat(PixelFormat::RGB, DataType::UInt8);
/// 8-bit RGBA.
constexpr ImageFormat rgba8 = ImageFormat(PixelFormat::RGBA, DataType::UInt8);
/// 8-bit BGR.
constexpr ImageFormat bgr8 = ImageFormat(PixelFormat::BGR, DataType::UInt8);
/// 8-bit BGRA.
constexpr ImageFormat bgra8 = ImageFormat(PixelFormat::BGRA, DataType::UInt8);
/// 32-bit float RGB.
constexpr ImageFormat rgb32f = ImageFormat(PixelFormat::RGB, DataType::Float32);
/// 32-bit float RGBA.
constexpr ImageFormat rgba32f = ImageFormat(PixelFormat::RGBA, DataType::Float32);
} // namespace fmt

} // namespace iml

#endif