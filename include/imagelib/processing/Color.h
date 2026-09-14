#pragma once
#define IMAGELIB_PROCESSING_COLOR_H_
// imagelib/processing/Color.h
//
// Color math: per-channel tone transforms (sRGB <-> linear, gamma) and color
// space conversions (RGB <-> HSV/HSL/XYZ/LAB), alpha compositing and
// luminance. Every conversion operates on normalized [0,1] channel values and
// returns a Pixel whose channels carry the target space's components.

#include "imagelib/processing/Pixel.h"

#include <cmath>

namespace iml {
namespace proc {
namespace color {

// ---------------------------------------------------------------------------
// Tone mapping (per scalar channel in [0,1]).
// ---------------------------------------------------------------------------

/// sRGB encoded -> linear light (sRGB transfer function, D65).
float srgbToLinear(float c) noexcept;

/// Linear light -> sRGB encoded.
float linearToSrgb(float c) noexcept;

/// Gamma encode (display): out = v^(1/gamma). gamma <= 0 returns v unchanged.
float gammaEncode(float c, float gamma) noexcept;

/// Gamma decode (linearize): out = v^gamma.
float gammaDecode(float c, float gamma) noexcept;

// ---------------------------------------------------------------------------
// Luminance and grayscale.
// ---------------------------------------------------------------------------

/// Rec.709 relative luminance from linear RGB.
float luminance(float r, float g, float b) noexcept;

/// Convert to grayscale (dst[*,*,0] = luminance; other channels are copied).
/// Convert to grayscale (rec.709 luma into the gray channel, alpha preserved).
void toGray(const ConstImageView& src, ImageView dst);

/// Row-parallel toGray (dynamic policy; serial for tiny images).
void toGray(const ConstImageView& src, ImageView dst,
            const ExecutionPolicy& policy);

// ---------------------------------------------------------------------------
// Alpha.
// ---------------------------------------------------------------------------

/// Multiply RGB by alpha (straight -> premultiplied).
Pixel premultiply(const Pixel& p) noexcept;

/// Divide RGB by alpha (premultiplied -> straight). Unpremultplies only when
/// alpha > 0, otherwise returns black.
Pixel unpremultiply(const Pixel& p) noexcept;

/// Standard source-over blend over background `bg` (straight alpha).
Pixel blendOver(const Pixel& bg, const Pixel& fg) noexcept;

// ---------------------------------------------------------------------------
// RGB <-> HSV  (hue in [0,1]; s, v in [0,1]).
// ---------------------------------------------------------------------------

Pixel rgbToHsv(const Pixel& p) noexcept;

Pixel hsvToRgb(const Pixel& p) noexcept;

// ---------------------------------------------------------------------------
// RGB <-> HSL  (hue in [0,1]; s, l in [0,1]).
// ---------------------------------------------------------------------------

Pixel rgbToHsl(const Pixel& p) noexcept;

Pixel hslToRgb(const Pixel& p) noexcept;

// ---------------------------------------------------------------------------
// RGB <-> XYZ (D65).
// ---------------------------------------------------------------------------

Pixel xyzToRgb(const Pixel& xyz) noexcept;

Pixel rgbToXyz(const Pixel& p) noexcept;

// ---------------------------------------------------------------------------
// XYZ <-> Lab (D65 white point). L in [0,100], a/b roughly in [-128,127].
// ---------------------------------------------------------------------------

namespace detail {
constexpr float labE = 216.f / 24389.f;      // (6/29)^3
constexpr float labK = 24389.f / 27.f;       // (29/3)^3
constexpr float wx = 0.95047f, wy = 1.0f, wz = 1.08883f;

inline constexpr float labF(float t) noexcept {
    return t > labE ? std::pow(t, 1.f / 3.f) : (labK * t + 16.f) / 116.f;
}
inline constexpr float labFInv(float f) noexcept {
    const float f3 = f * f * f;
    return f3 > labE ? f3 : (116.f * f - 16.f) / labK;
}
} // namespace detail

Pixel xyzToLab(const Pixel& xyz) noexcept;

Pixel labToXyz(const Pixel& lab) noexcept;

/// Convenience: sRGB-pixel -> Lab.
Pixel rgbToLab(const Pixel& p) noexcept;
/// Convenience: Lab -> sRGB-pixel.
Pixel labToRgb(const Pixel& lab) noexcept;

} // namespace color
} // namespace proc
} // namespace iml