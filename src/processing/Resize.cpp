#include "imagelib/processing/Resize.h"

#include <cmath>

namespace iml {
namespace proc {
namespace resize {

void resize(const ConstImageView& src, ImageView dst, Filter f,
            conv::BorderMode border) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("resize::resize: invalid view");
    const float sw = static_cast<float>(src.width());
    const float sh = static_cast<float>(src.height());
    const uint32 dw = dst.width(), dh = dst.height();
    const uint32 ch = std::min<uint32>(src.channels(), dst.channels());

    if (f == Filter::Nearest) {
        for (uint32 y = 0; y < dh; ++y) {
            const int32 sy = static_cast<int32>((y * sh) / static_cast<float>(dh));
            for (uint32 x = 0; x < dw; ++x) {
                const int32 sx = static_cast<int32>((x * sw) / static_cast<float>(dw));
                for (uint32 c = 0; c < ch; ++c)
                    detail::writeNorm(dst, x, y, c,
                                      conv::sampleNorm(src, sx, sy, c, border));
            }
        }
        return;
    }

    const float wScale = sw / static_cast<float>(dw);
    const float hScale = sh / static_cast<float>(dh);
    for (uint32 y = 0; y < dh; ++y) {
        const float gy = (y + 0.5f) * hScale - 0.5f;
        const int32 y0 = static_cast<int32>(std::floor(gy));
        const float fy = gy - static_cast<float>(y0);
        for (uint32 x = 0; x < dw; ++x) {
            const float gx = (x + 0.5f) * wScale - 0.5f;
            const int32 x0 = static_cast<int32>(std::floor(gx));
            const float fx = gx - static_cast<float>(x0);
            for (uint32 c = 0; c < ch; ++c) {
                const float v00 = conv::sampleNorm(src, x0, y0, c, border);
                const float v10 = conv::sampleNorm(src, x0 + 1, y0, c, border);
                const float v01 = conv::sampleNorm(src, x0, y0 + 1, c, border);
                const float v11 = conv::sampleNorm(src, x0 + 1, y0 + 1, c, border);
                const float top = v00 + (v10 - v00) * fx;
                const float bot = v01 + (v11 - v01) * fx;
                detail::writeNorm(dst, x, y, c, top + (bot - top) * fy);
            }
        }
    }
}

void resize(const ConstImageView& src, ImageView dst, Filter f,
            conv::BorderMode border, const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("resize::resize: invalid view");
    const float sw = static_cast<float>(src.width());
    const float sh = static_cast<float>(src.height());
    const uint32 dw = dst.width(), dh = dst.height();
    const uint32 ch = std::min<uint32>(src.channels(), dst.channels());
    const bool nearest = (f == Filter::Nearest);
    if (!wantsParallel(policy, static_cast<size_t>(dw) * dh * ch, 16384)) {
        resize(src, dst, f, border);
        return;
    }
    const float wScale = sw / static_cast<float>(dw);
    const float hScale = sh / static_cast<float>(dh);
    const uint32 width = dw, height = dh;
    parallelForRows(height, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        if (nearest) {
            const int32 sy = static_cast<int32>((y * sh) / static_cast<float>(height));
            for (uint32 x = 0; x < width; ++x) {
                const int32 sx = static_cast<int32>((x * sw) / static_cast<float>(width));
                for (uint32 c = 0; c < ch; ++c)
                    detail::writeNorm(dst, x, y, c,
                                      conv::sampleNorm(src, sx, sy, c, border));
            }
            return;
        }
        const float gy = (y + 0.5f) * hScale - 0.5f;
        const int32 y0 = static_cast<int32>(std::floor(gy));
        const float fy = gy - static_cast<float>(y0);
        for (uint32 x = 0; x < width; ++x) {
            const float gx = (x + 0.5f) * wScale - 0.5f;
            const int32 x0 = static_cast<int32>(std::floor(gx));
            const float fx = gx - static_cast<float>(x0);
            for (uint32 c = 0; c < ch; ++c) {
                const float v00 = conv::sampleNorm(src, x0, y0, c, border);
                const float v10 = conv::sampleNorm(src, x0 + 1, y0, c, border);
                const float v01 = conv::sampleNorm(src, x0, y0 + 1, c, border);
                const float v11 = conv::sampleNorm(src, x0 + 1, y0 + 1, c, border);
                const float top = v00 + (v10 - v00) * fx;
                const float bot = v01 + (v11 - v01) * fx;
                detail::writeNorm(dst, x, y, c, top + (bot - top) * fy);
            }
        }
    }, policy);
}

} // namespace resize
} // namespace proc
} // namespace iml