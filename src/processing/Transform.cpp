#include "imagelib/processing/Transform.h"

namespace iml {
namespace proc {
namespace transform {

void flipVertical(const ConstImageView& src, ImageView dst) {
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("transform::flipVertical: shape mismatch");
    const uint32 w = src.width(), h = src.height(), ch = src.channels();
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            for (uint32 c = 0; c < ch; ++c)
                detail::writeNorm(dst, x, y, c, detail::readNorm(src, x, h - 1 - y, c));
}

void flipVertical(const ConstImageView& src, ImageView dst,
                  const ExecutionPolicy& p) {
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("transform::flipVertical: shape mismatch");
    const uint32 w = src.width(), h = src.height(), ch = src.channels();
    if (!wantsParallel(p, static_cast<size_t>(w) * h * ch, 8192)) { flipVertical(src, dst); return; }
    const uint32 width = w, height = h;
    parallelForRows(height, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x)
            for (uint32 c = 0; c < ch; ++c)
                detail::writeNorm(dst, x, y, c, detail::readNorm(src, x, height - 1 - y, c));
    }, p);
}

void flipHorizontal(const ConstImageView& src, ImageView dst) {
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("transform::flipHorizontal: shape mismatch");
    const uint32 w = src.width(), h = src.height(), ch = src.channels();
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            for (uint32 c = 0; c < ch; ++c)
                detail::writeNorm(dst, x, y, c, detail::readNorm(src, w - 1 - x, y, c));
}

void flipHorizontal(const ConstImageView& src, ImageView dst,
                    const ExecutionPolicy& p) {
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("transform::flipHorizontal: shape mismatch");
    const uint32 w = src.width(), h = src.height(), ch = src.channels();
    if (!wantsParallel(p, static_cast<size_t>(w) * h * ch, 8192)) { flipHorizontal(src, dst); return; }
    const uint32 width = w, height = h;
    parallelForRows(height, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x)
            for (uint32 c = 0; c < ch; ++c)
                detail::writeNorm(dst, x, y, c, detail::readNorm(src, width - 1 - x, y, c));
    }, p);
}

void rotate180(const ConstImageView& src, ImageView dst) {
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("transform::rotate180: shape mismatch");
    const uint32 w = src.width(), h = src.height(), ch = src.channels();
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            for (uint32 c = 0; c < ch; ++c)
                detail::writeNorm(dst, x, y, c, detail::readNorm(src, w - 1 - x, h - 1 - y, c));
}

void rotate180(const ConstImageView& src, ImageView dst,
               const ExecutionPolicy& p) {
    if (src.width() != dst.width() || src.height() != dst.height() || src.channels() != dst.channels())
        throw InvalidParameterError("transform::rotate180: shape mismatch");
    const uint32 w = src.width(), h = src.height(), ch = src.channels();
    if (!wantsParallel(p, static_cast<size_t>(w) * h * ch, 8192)) { rotate180(src, dst); return; }
    const uint32 width = w, height = h;
    parallelForRows(height, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x)
            for (uint32 c = 0; c < ch; ++c)
                detail::writeNorm(dst, x, y, c, detail::readNorm(src, width - 1 - x, height - 1 - y, c));
    }, p);
}

void rotate90(const ConstImageView& src, ImageView dst) {
    if (dst.width() != src.height() || dst.height() != src.width() || src.channels() != dst.channels())
        throw InvalidParameterError("transform::rotate90: shape mismatch");
    const uint32 sh = src.height(), ch = src.channels();
    for (uint32 y = 0; y < dst.height(); ++y)
        for (uint32 x = 0; x < dst.width(); ++x)
            for (uint32 c = 0; c < ch; ++c)
                detail::writeNorm(dst, x, y, c, detail::readNorm(src, y, sh - 1 - x, c));
}

void rotate270(const ConstImageView& src, ImageView dst) {
    if (dst.width() != src.height() || dst.height() != src.width() || src.channels() != dst.channels())
        throw InvalidParameterError("transform::rotate270: shape mismatch");
    const uint32 sw = src.width(), ch = src.channels();
    for (uint32 y = 0; y < dst.height(); ++y)
        for (uint32 x = 0; x < dst.width(); ++x)
            for (uint32 c = 0; c < ch; ++c)
                detail::writeNorm(dst, x, y, c, detail::readNorm(src, sw - 1 - y, x, c));
}

void transpose(const ConstImageView& src, ImageView dst) {
    if (dst.width() != src.height() || dst.height() != src.width() || src.channels() != dst.channels())
        throw InvalidParameterError("transform::transpose: shape mismatch");
    const uint32 ch = src.channels();
    for (uint32 y = 0; y < dst.height(); ++y)
        for (uint32 x = 0; x < dst.width(); ++x)
            for (uint32 c = 0; c < ch; ++c)
                detail::writeNorm(dst, x, y, c, detail::readNorm(src, y, x, c));
}

void copyRoi(const ConstImageView& src, ImageView dst,
             uint32 x0, uint32 y0, uint32 w, uint32 h) {
    if (x0 + w > src.width() || y0 + h > src.height())
        throw InvalidParameterError("transform::copyRoi: ROI out of bounds");
    if (dst.width() < w || dst.height() < h || dst.channels() != src.channels())
        throw InvalidParameterError("transform::copyRoi: dst too small or channel mismatch");
    const uint32 ch = src.channels();
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            for (uint32 c = 0; c < ch; ++c)
                detail::writeNorm(dst, x, y, c, detail::readNorm(src, x0 + x, y0 + y, c));
}

void copyRoi(const ConstImageView& src, ImageView dst,
             uint32 x0, uint32 y0, uint32 w, uint32 h,
             const ExecutionPolicy& p) {
    if (x0 + w > src.width() || y0 + h > src.height())
        throw InvalidParameterError("transform::copyRoi: ROI out of bounds");
    if (dst.width() < w || dst.height() < h || dst.channels() != src.channels())
        throw InvalidParameterError("transform::copyRoi: dst too small or channel mismatch");
    const uint32 ch = src.channels();
    if (!wantsParallel(p, static_cast<size_t>(w) * h * ch, 8192)) {
        copyRoi(src, dst, x0, y0, w, h);
        return;
    }
    const uint32 rx0 = x0, ry0 = y0, width = w, height = h;
    parallelForRows(height, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x)
            for (uint32 c = 0; c < ch; ++c)
                detail::writeNorm(dst, x, y, c, detail::readNorm(src, rx0 + x, ry0 + y, c));
    }, p);
}

} // namespace transform
} // namespace proc
} // namespace iml