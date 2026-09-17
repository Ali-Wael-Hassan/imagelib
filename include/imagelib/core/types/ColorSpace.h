#pragma once
#ifndef IMAGELIB_CORE_COLORSPACE_H_
#define IMAGELIB_CORE_COLORSPACE_H_
/// @file
/// Color space enumeration and string conversion.

#include "imagelib/core/types/FixedTypes.h"

namespace iml {

/// Color space of an image.
enum class ColorSpace : uint8 {
    /// Color space is not known.
    Unknown = 0,
    /// Linear RGBA light.
    Linear,
    /// sRGB encoded.
    SRGB,
    /// Luminance-only.
    Gray,
    /// Hue, saturation, value.
    HSV,
    /// Hue, saturation, lightness.
    HSL,
    /// CIE XYZ tristimulus values.
    XYZ,
    /// CIELAB color space.
    LAB,
    /// Luma with blue and red chroma.
    YCbCr,
};

/// Returns a printable name for a color space.
/// @param c The color space to convert.
/// @return A NUL-terminated string naming `c`, or "Unknown".
constexpr const char* toString(ColorSpace c) noexcept {
    switch (c) {
    case ColorSpace::Unknown:
        return "Unknown";
    case ColorSpace::Linear:
        return "Linear";
    case ColorSpace::SRGB:
        return "sRGB";
    case ColorSpace::Gray:
        return "Gray";
    case ColorSpace::HSV:
        return "HSV";
    case ColorSpace::HSL:
        return "HSL";
    case ColorSpace::XYZ:
        return "XYZ";
    case ColorSpace::LAB:
        return "LAB";
    case ColorSpace::YCbCr:
        return "YCbCr";
    }
    return "Unknown";
}

} // namespace iml

#endif