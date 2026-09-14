#include "imagelib/processing/Filters.h"

#include <algorithm>
#include <vector>

namespace iml {
namespace proc {
namespace filter {

void checkShape(const ConstImageView& src, ImageView dst) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("filter: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("filter: dimension/channel mismatch");
}

void invert(const ConstImageView& src, ImageView dst) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [](const Pixel& p) {
        return Pixel(1.f - p.r, 1.f - p.g, 1.f - p.b, p.a);
    });
}

void brightness(const ConstImageView& src, ImageView dst, float delta) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [d = delta](const Pixel& p) {
        return Pixel(p.r + d, p.g + d, p.b + d, p.a);
    });
}

void contrast(const ConstImageView& src, ImageView dst, float factor) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [k = factor](const Pixel& p) {
        return Pixel((p.r - 0.5f) * k + 0.5f, (p.g - 0.5f) * k + 0.5f,
                     (p.b - 0.5f) * k + 0.5f, p.a);
    });
}

void linear(const ConstImageView& src, ImageView dst, float scale, float offset) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [s = scale, o = offset](const Pixel& p) {
        return Pixel(p.r * s + o, p.g * s + o, p.b * s + o, p.a);
    });
}

void gammaCorrect(const ConstImageView& src, ImageView dst, float gamma) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [g = gamma](const Pixel& p) {
        return Pixel(color::gammaEncode(p.r, g), color::gammaEncode(p.g, g),
                     color::gammaEncode(p.b, g), p.a);
    });
}

void threshold(const ConstImageView& src, ImageView dst, float t,
               bool above) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [t, above](const Pixel& p) {
        const float l = p.r; // channel 0 sample
        const bool cond = above ? (l >= t) : (l < t);
        const float v = cond ? 1.f : 0.f;
        return Pixel(v, v, v, p.a);
    });
}

void toLinear(const ConstImageView& src, ImageView dst) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [](const Pixel& p) {
        return Pixel(color::srgbToLinear(p.r), color::srgbToLinear(p.g),
                     color::srgbToLinear(p.b), p.a);
    });
}

void toSrgb(const ConstImageView& src, ImageView dst) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [](const Pixel& p) {
        return Pixel(color::linearToSrgb(p.r), color::linearToSrgb(p.g),
                     color::linearToSrgb(p.b), p.a);
    });
}

void desaturate(const ConstImageView& src, ImageView dst) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [](const Pixel& p) {
        const float l = p.luminance();
        return Pixel(l, l, l, p.a);
    });
}

void median(const ConstImageView& src, ImageView dst, int radius,
            conv::BorderMode border) {
    checkShape(src, dst);
    if (radius < 1 || radius > 128)
        throw InvalidParameterError("filter::median: radius out of range");
    const uint32 w = src.width(), h = src.height();
    const uint32 ch = src.channels();
    const int32 r = radius;
    std::vector<float> buf(static_cast<size_t>((2 * r + 1) * (2 * r + 1)));
    for (uint32 y = 0; y < h; ++y) {
        for (uint32 x = 0; x < w; ++x) {
            for (uint32 c = 0; c < ch; ++c) {
                size_t n = 0;
                for (int32 dy = -r; dy <= r; ++dy)
                    for (int32 dx = -r; dx <= r; ++dx)
                        buf[n++] = conv::sampleNorm(src, static_cast<int32>(x) + dx,
                                                    static_cast<int32>(y) + dy, c, border);
                std::nth_element(buf.begin(), buf.begin() + n / 2, buf.begin() + n);
                detail::writeNorm(dst, x, y, c, buf[n / 2]);
            }
        }
    }
}

void invert(const ConstImageView& src, ImageView dst, const ExecutionPolicy& p) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [](const Pixel& q) {
        return Pixel(1.f - q.r, 1.f - q.g, 1.f - q.b, q.a);
    }, p);
}

void brightness(const ConstImageView& src, ImageView dst, float delta,
                const ExecutionPolicy& p) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [d = delta](const Pixel& q) {
        return Pixel(q.r + d, q.g + d, q.b + d, q.a);
    }, p);
}

void contrast(const ConstImageView& src, ImageView dst, float factor,
              const ExecutionPolicy& p) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [k = factor](const Pixel& q) {
        return Pixel((q.r - 0.5f) * k + 0.5f, (q.g - 0.5f) * k + 0.5f,
                     (q.b - 0.5f) * k + 0.5f, q.a);
    }, p);
}

void threshold(const ConstImageView& src, ImageView dst, float t,
               const ExecutionPolicy& p) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [t](const Pixel& q) {
        const float v = q.r >= t ? 1.f : 0.f;
        return Pixel(v, v, v, q.a);
    }, p);
}

void desaturate(const ConstImageView& src, ImageView dst,
                const ExecutionPolicy& p) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [](const Pixel& q) {
        const float l = q.luminance();
        return Pixel(l, l, l, q.a);
    }, p);
}

void linear(const ConstImageView& src, ImageView dst, float scale,
            float offset, const ExecutionPolicy& p) {
    checkShape(src, dst);
    detail::mapPixels(src, dst, [s = scale, o = offset](const Pixel& q) {
        return Pixel(q.r * s + o, q.g * s + o, q.b * s + o, q.a);
    }, p);
}

} // namespace filter
} // namespace proc
} // namespace iml