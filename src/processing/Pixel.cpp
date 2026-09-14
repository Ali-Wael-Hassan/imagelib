#include "imagelib/processing/Pixel.h"

#include <algorithm>
#include <cmath>

namespace iml {

Pixel& Pixel::operator+=(const Pixel& o) noexcept { r += o.r; g += o.g; b += o.b; a += o.a; return *this; }
Pixel& Pixel::operator-=(const Pixel& o) noexcept { r -= o.r; g -= o.g; b -= o.b; a -= o.a; return *this; }
Pixel& Pixel::operator*=(const Pixel& o) noexcept { r *= o.r; g *= o.g; b *= o.b; a *= o.a; return *this; }
Pixel& Pixel::operator*=(float s) noexcept { r *= s; g *= s; b *= s; a *= s; return *this; }
Pixel& Pixel::operator/=(float s) noexcept { r /= s; g /= s; b /= s; a /= s; return *this; }

float Pixel::luminance() const noexcept { return 0.2126f * r + 0.7152f * g + 0.0722f * b; }

Pixel clamp01(const Pixel& p) noexcept {
    return {std::min(1.f, std::max(0.f, p.r)),
            std::min(1.f, std::max(0.f, p.g)),
            std::min(1.f, std::max(0.f, p.b)),
            std::min(1.f, std::max(0.f, p.a))};
}

Pixel lerp(const Pixel& a, const Pixel& b, float t) noexcept {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}

namespace proc {
namespace detail {

float c01(float v) noexcept { return std::min(1.f, std::max(0.f, v)); }

float clampf(float v, float lo, float hi) noexcept {
    return std::min(hi, std::max(lo, v));
}

void writeNorm(ImageView dst, int32 x, int32 y, uint32 c, float v) {
    switch (dst.dataType()) {
        case DataType::UInt8:
            dst.setSample<uint8>(x, y, c, static_cast<uint8>(c01(v) * 255.f + 0.5f));
            break;
        case DataType::UInt16:
            dst.setSample<uint16>(x, y, c, static_cast<uint16>(c01(v) * 65535.f + 0.5f));
            break;
        case DataType::UInt32: {
            const double q = clampf(v, 0.f, 1.f) * 4294967295.0 + 0.5;
            dst.setSample<uint32>(x, y, c, static_cast<uint32>(q));
            break;
        }
        case DataType::Float32:
            dst.setSample<float>(x, y, c, v);
            break;
        default:
            throw UnsupportedFormatError("proc::writeNorm: unsupported destination type");
    }
}

float readNorm(const ConstImageView& src, int32 x, int32 y, uint32 c) {
    switch (src.dataType()) {
        case DataType::UInt8:  return src.getSample<uint8>(x, y, c) * (1.f / 255.f);
        case DataType::UInt16: return src.getSample<uint16>(x, y, c) * (1.f / 65535.f);
        case DataType::UInt32: return src.getSample<uint32>(x, y, c) * (1.f / 4294967295.f);
        case DataType::Float32:return src.getSample<float>(x, y, c);
        default: throw UnsupportedFormatError("proc::readNorm: unsupported source type");
    }
}

Pixel readPixel(const ConstImageView& src, int32 x, int32 y) {
    const uint32 c = src.channels();
    switch (src.pixelFormat()) {
        case PixelFormat::Gray:
            return Pixel(detail::readNorm(src, x, y, 0), detail::readNorm(src, x, y, 0),
                         detail::readNorm(src, x, y, 0), 1.f);
        case PixelFormat::GrayAlpha:
            return Pixel(detail::readNorm(src, x, y, 0), detail::readNorm(src, x, y, 0),
                         detail::readNorm(src, x, y, 0), detail::readNorm(src, x, y, 1));
        case PixelFormat::RGB:
            return Pixel(detail::readNorm(src, x, y, 0), detail::readNorm(src, x, y, 1),
                         detail::readNorm(src, x, y, 2), 1.f);
        case PixelFormat::BGR:
            return Pixel(detail::readNorm(src, x, y, 2), detail::readNorm(src, x, y, 1),
                         detail::readNorm(src, x, y, 0), 1.f);
        case PixelFormat::RGBA:
            return Pixel(detail::readNorm(src, x, y, 0), detail::readNorm(src, x, y, 1),
                         detail::readNorm(src, x, y, 2), detail::readNorm(src, x, y, 3));
        case PixelFormat::BGRA:
            return Pixel(detail::readNorm(src, x, y, 2), detail::readNorm(src, x, y, 1),
                         detail::readNorm(src, x, y, 0), detail::readNorm(src, x, y, 3));
        default:
            break;
    }
    (void)c;
    throw UnsupportedFormatError("proc::readPixel: unsupported pixel format");
}

void writeRGBA(const Pixel& p, ImageView dst, int32 x, int32 y) {
    switch (dst.pixelFormat()) {
        case PixelFormat::Gray:
        case PixelFormat::GrayAlpha:
            writeNorm(dst, x, y, 0, p.r); // gray layouts write luminance into channel 0
            break;
        case PixelFormat::RGB:
            writeNorm(dst, x, y, 0, p.r);
            writeNorm(dst, x, y, 1, p.g);
            writeNorm(dst, x, y, 2, p.b);
            break;
        case PixelFormat::BGR:
            writeNorm(dst, x, y, 0, p.b);
            writeNorm(dst, x, y, 1, p.g);
            writeNorm(dst, x, y, 2, p.r);
            break;
        case PixelFormat::RGBA:
            writeNorm(dst, x, y, 0, p.r);
            writeNorm(dst, x, y, 1, p.g);
            writeNorm(dst, x, y, 2, p.b);
            writeNorm(dst, x, y, 3, p.a);
            break;
        case PixelFormat::BGRA:
            writeNorm(dst, x, y, 0, p.b);
            writeNorm(dst, x, y, 1, p.g);
            writeNorm(dst, x, y, 2, p.r);
            writeNorm(dst, x, y, 3, p.a);
            break;
        default:
            throw UnsupportedFormatError("proc::writeRGBA: unsupported destination format");
    }
}

} // namespace detail
} // namespace proc
} // namespace iml