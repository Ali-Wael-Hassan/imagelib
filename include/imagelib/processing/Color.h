#pragma once
#define IMAGELIB_PROCESSING_COLOR_H_
/// @file Color.h
/// Color math: per-channel tone transforms (sRGB <-> linear, gamma), color
/// space conversions (RGB <-> HSV/HSL/XYZ/LAB), alpha compositing and
/// luminance, all on normalized [0,1] channel values.

#include "imagelib/core/image/Pixel.h"
#include "imagelib/core/ExecutionPolicy.h"
#include "imagelib/threading/ParallelFor.h"

#include <cmath>

namespace iml {
namespace proc {
namespace color {

/// sRGB encoded -> linear light (sRGB transfer function, D65).
/// @param c sRGB channel in [0,1].
/// @return Linear light in [0,1].
float srgbToLinear(float c) noexcept;

/// Linear light -> sRGB encoded.
/// @param c Linear channel in [0,1].
/// @return sRGB-encoded channel in [0,1].
float linearToSrgb(float c) noexcept;

/// Gamma encode (display): out = v^(1/gamma). gamma <= 0 returns v unchanged.
/// @param c Channel in [0,1].
/// @param gamma Display gamma.
/// @return Encoded channel.
float gammaEncode(float c, float gamma) noexcept;

/// Gamma decode (linearize): out = v^gamma.
/// @param c Encoded channel in [0,1].
/// @param gamma Display gamma.
/// @return Linearized channel.
float gammaDecode(float c, float gamma) noexcept;

/// Rec.709 relative luminance from linear RGB.
/// @param r,g,b Linear RGB channels.
/// @return Rec.709 relative luminance.
float luminance(float r, float g, float b) noexcept;

/// Convert to grayscale: rec.709 luma in the gray channel, alpha preserved.
/// @param src Source image view.
/// @param dst Destination image view (same dimensions).
void toGray(const ConstImageView& src, ImageView dst);

/// Row-parallel toGray (dynamic policy; serial for tiny images).
/// @param src Source image view.
/// @param dst Destination image view (same dimensions).
/// @param policy Execution policy governing parallelism.
void toGray(const ConstImageView& src, ImageView dst, const ExecutionPolicy& policy);

/// Multiply RGB by alpha (straight -> premultiplied).
/// @param p Straight-alpha pixel.
/// @return Premultiplied-alpha pixel.
Pixel premultiply(const Pixel& p) noexcept;

/// Divide RGB by alpha (premultiplied -> straight). Unpremultplies only when
/// alpha > 0, otherwise returns black.
/// @param p Premultiplied-alpha pixel.
/// @return Straight-alpha pixel.
Pixel unpremultiply(const Pixel& p) noexcept;

/// Standard source-over blend over background `bg` (straight alpha).
/// @param bg Background pixel.
/// @param fg Foreground pixel.
/// @return Source-over composited pixel.
Pixel blendOver(const Pixel& bg, const Pixel& fg) noexcept;

/// Convert RGB to HSV (hue, s, v in [0,1]).
/// @param p Canonical RGB pixel.
/// @return Canonical pixel with HSV in the RGB channels.
Pixel rgbToHsv(const Pixel& p) noexcept;

/// Convert HSV to RGB (hue, s, v in [0,1]).
/// @param p Canonical HSV pixel.
/// @return Canonical pixel with RGB in the RGB channels.
Pixel hsvToRgb(const Pixel& p) noexcept;

/// Convert RGB to HSL (hue, s, l in [0,1]).
/// @param p Canonical RGB pixel.
/// @return Canonical pixel with HSL in the RGB channels.
Pixel rgbToHsl(const Pixel& p) noexcept;

/// Convert HSL to RGB (hue, s, l in [0,1]).
/// @param p Canonical HSL pixel.
/// @return Canonical pixel with RGB in the RGB channels.
Pixel hslToRgb(const Pixel& p) noexcept;

/// Convert XYZ to RGB (D65).
/// @param xyz Canonical pixel with XYZ in the RGB channels.
/// @return Canonical sRGB pixel.
Pixel xyzToRgb(const Pixel& xyz) noexcept;

/// Convert RGB to XYZ (D65).
/// @param p Canonical sRGB pixel.
/// @return Canonical pixel with XYZ in the RGB channels.
Pixel rgbToXyz(const Pixel& p) noexcept;

/// Convert XYZ to Lab (D65 white point; L in [0,100], a/b in [-128,127]).
/// @param xyz Canonical pixel with XYZ in the RGB channels.
/// @return Canonical pixel with Lab in the RGB channels.
Pixel xyzToLab(const Pixel& xyz) noexcept;

/// Convert Lab to XYZ (D65 white point).
/// @param lab Canonical pixel with Lab in the RGB channels.
/// @return Canonical pixel with XYZ in the RGB channels.
Pixel labToXyz(const Pixel& lab) noexcept;

namespace detail {
constexpr float labE = 216.f / 24389.f;
constexpr float labK = 24389.f / 27.f;
constexpr float wx = 0.95047f, wy = 1.0f, wz = 1.08883f;

inline constexpr float labF(float t) noexcept {
    return t > labE ? std::pow(t, 1.f / 3.f) : (labK * t + 16.f) / 116.f;
}
inline constexpr float labFInv(float f) noexcept {
    const float f3 = f * f * f;
    return f3 > labE ? f3 : (116.f * f - 16.f) / labK;
}
} // namespace detail

/// Convenience: sRGB-pixel -> Lab.
/// @param p Canonical sRGB pixel.
/// @return Canonical pixel with Lab in the RGB channels.
Pixel rgbToLab(const Pixel& p) noexcept;
/// Convenience: Lab -> sRGB-pixel.
/// @param lab Canonical pixel with Lab in the RGB channels.
/// @return Canonical sRGB pixel.
Pixel labToRgb(const Pixel& lab) noexcept;

} // namespace color
} // namespace proc
} // namespace iml