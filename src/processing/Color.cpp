#include "imagelib/processing/Color.h"

#include <algorithm>
#include <cmath>

namespace iml {
namespace proc {
namespace color {

namespace detail {
using ::iml::proc::detail::c01;
using ::iml::proc::detail::clampf;
using ::iml::proc::detail::readPixel;
using ::iml::proc::detail::writeRGBA;
} // namespace detail

float srgbToLinear(float c) noexcept {
    c = detail::c01(c);
    return c <= 0.04045f ? c * (1.f / 12.92f) : std::pow((c + 0.055f) * (1.f / 1.055f), 2.4f);
}

float linearToSrgb(float c) noexcept {
    c = detail::c01(c);
    return c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 1.f / 2.4f) - 0.055f;
}

float gammaEncode(float c, float gamma) noexcept {
    if (gamma <= 0.f) return c;
    return std::pow(detail::c01(c), 1.f / gamma);
}

float gammaDecode(float c, float gamma) noexcept {
    if (gamma <= 0.f) return c;
    return std::pow(detail::c01(c), gamma);
}

float luminance(float r, float g, float b) noexcept {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

void toGray(const ConstImageView& src, ImageView dst) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("color::toGray: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height())
        throw InvalidParameterError("color::toGray: dimension mismatch");
    const uint32 w = src.width(), h = src.height();
    for (uint32 y = 0; y < h; ++y) {
        for (uint32 x = 0; x < w; ++x) {
            const Pixel p = detail::readPixel(src, x, y);
            const float l = luminance(p.r, p.g, p.b);
            detail::writeRGBA(Pixel(l, l, l, p.a), dst, x, y);
        }
    }
}

void toGray(const ConstImageView& src, ImageView dst,
            const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("color::toGray: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height())
        throw InvalidParameterError("color::toGray: dimension mismatch");
    const uint32 w = src.width(), h = src.height();
    if (!wantsParallel(policy, static_cast<size_t>(w) * h, 4096)) {
        toGray(src, dst);
        return;
    }
    const uint32 width = w;
    parallelForRows(h, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x) {
            const Pixel p = detail::readPixel(src, x, y);
            const float l = luminance(p.r, p.g, p.b);
            detail::writeRGBA(Pixel(l, l, l, p.a), dst, x, y);
        }
    }, policy);
}

Pixel premultiply(const Pixel& p) noexcept { return {p.r * p.a, p.g * p.a, p.b * p.a, p.a}; }

Pixel unpremultiply(const Pixel& p) noexcept {
    const float inv = p.a > 0.f ? 1.f / p.a : 0.f;
    return {p.r * inv, p.g * inv, p.b * inv, p.a};
}

Pixel blendOver(const Pixel& bg, const Pixel& fg) noexcept {
    const float a = detail::c01(fg.a);
    const float inv = 1.f - a;
    return {fg.r * a + bg.r * inv,
            fg.g * a + bg.g * inv,
            fg.b * a + bg.b * inv,
            a + bg.a * inv};
}

Pixel rgbToHsv(const Pixel& p) noexcept {
    const float r = detail::c01(p.r), g = detail::c01(p.g), b = detail::c01(p.b);
    const float mx = std::max(r, std::max(g, b));
    const float mn = std::min(r, std::min(g, b));
    const float d = mx - mn;
    float h = 0.f;
    if (d != 0.f) {
        if (mx == r)      h = std::fmod((g - b) / d, 6.f);
        else if (mx == g) h = (b - r) / d + 2.f;
        else              h = (r - g) / d + 4.f;
        h *= (1.f / 6.f);
        if (h < 0.f) h += 1.f;
    }
    const float s = mx == 0.f ? 0.f : d / mx;
    return {h, s, mx, p.a};
}

Pixel hsvToRgb(const Pixel& p) noexcept {
    float h = p.r - std::floor(p.r); // wrap to [0,1)
    const float s = detail::c01(p.g);
    const float v = detail::c01(p.b);
    const float i = std::floor(h * 6.f);
    const float f = h * 6.f - i;
    const float pv = v * (1.f - s);
    const float q = v * (1.f - s * f);
    const float t = v * (1.f - s * (1.f - f));
    switch (static_cast<int>(i) % 6) {
        case 0: return {v, t, pv, p.a};
        case 1: return {q, v, pv, p.a};
        case 2: return {pv, v, t, p.a};
        case 3: return {pv, q, v, p.a};
        case 4: return {t, pv, v, p.a};
        default: return {v, pv, q, p.a};
    }
}

Pixel rgbToHsl(const Pixel& p) noexcept {
    const float r = detail::c01(p.r), g = detail::c01(p.g), b = detail::c01(p.b);
    const float mx = std::max(r, std::max(g, b));
    const float mn = std::min(r, std::min(g, b));
    const float d = mx - mn;
    const float l = 0.5f * (mx + mn);
    float h = 0.f, s = 0.f;
    if (d != 0.f) {
        if (mx == r)      h = std::fmod((g - b) / d, 6.f);
        else if (mx == g) h = (b - r) / d + 2.f;
        else              h = (r - g) / d + 4.f;
        h *= (1.f / 6.f);
        if (h < 0.f) h += 1.f;
        s = d / (1.f - std::abs(2.f * l - 1.f));
    }
    return {h, s, l, p.a};
}

Pixel hslToRgb(const Pixel& p) noexcept {
    float h = p.r - std::floor(p.r);
    const float s = detail::c01(p.g);
    const float l = detail::c01(p.b);
    auto hue2rgb = [](float pv, float q, float t) -> float {
        if (t < 0.f) t += 1.f;
        if (t > 1.f) t -= 1.f;
        if (t < 1.f / 6.f) return pv + (q - pv) * 6.f * t;
        if (t < 1.f / 2.f) return q;
        if (t < 2.f / 3.f) return pv + (q - pv) * (2.f / 3.f - t) * 6.f;
        return pv;
    };
    if (s == 0.f) return {l, l, l, p.a};
    const float q = l < 0.5f ? l * (1.f + s) : l + s - l * s;
    const float pv = 2.f * l - q;
    return {hue2rgb(pv, q, h + 1.f / 3.f), hue2rgb(pv, q, h), hue2rgb(pv, q, h - 1.f / 3.f), p.a};
}

Pixel xyzToRgb(const Pixel& xyz) noexcept {
    const float x = xyz.r, y = xyz.g, z = xyz.b;
    return {detail::clampf(3.2404542f * x - 1.5371385f * y - 0.4985314f * z, 0.f, 1.f),
            detail::clampf(-0.9692660f * x + 1.8760108f * y + 0.0415560f * z, 0.f, 1.f),
            detail::clampf(0.0556434f * x - 0.2040259f * y + 1.0572252f * z, 0.f, 1.f),
            xyz.a};
}

Pixel rgbToXyz(const Pixel& p) noexcept {
    const float r = srgbToLinear(p.r), g = srgbToLinear(p.g), b = srgbToLinear(p.b);
    return {0.4124564f * r + 0.3575761f * g + 0.1804375f * b,
            0.2126729f * r + 0.7151522f * g + 0.0721750f * b,
            0.0193339f * r + 0.1191920f * g + 0.9503041f * b,
            p.a};
}

Pixel xyzToLab(const Pixel& xyz) noexcept {
    const float fx = detail::labF(xyz.r / detail::wx);
    const float fy = detail::labF(xyz.g / detail::wy);
    const float fz = detail::labF(xyz.b / detail::wz);
    return {116.f * fy - 16.f,
            500.f * (fx - fy),
            200.f * (fy - fz),
            xyz.a};
}

Pixel labToXyz(const Pixel& lab) noexcept {
    const float fy = (lab.r + 16.f) / 116.f;
    const float fx = fy + lab.g / 500.f;
    const float fz = fy - lab.b / 200.f;
    return {detail::wx * detail::labFInv(fx),
            detail::wy * detail::labFInv(fy),
            detail::wz * detail::labFInv(fz),
            lab.a};
}

Pixel rgbToLab(const Pixel& p) noexcept { return xyzToLab(rgbToXyz(p)); }
Pixel labToRgb(const Pixel& lab) noexcept { return xyzToRgb(labToXyz(lab)); }

} // namespace color
} // namespace proc
} // namespace iml