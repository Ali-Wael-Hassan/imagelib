#ifndef IMAGELIB_PROCESSING_PIXEL_H_
#error "Include Pixel.h, not this .tpp directly."
#endif

#include <cstddef>

#include "imagelib/processing/Pixel.h"

namespace iml {
namespace proc {
namespace detail {

/// Loop over every pixel, mapping canonical pixels through `fn`.
template <class Fn>
void mapPixels(const ConstImageView& src, ImageView dst, Fn&& fn) {
    if (!src.valid() || !dst.valid()) throw InvalidParameterError("proc::mapPixels: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height())
        throw InvalidParameterError("proc::mapPixels: dimension mismatch");
    const uint32 w = src.width(), h = src.height();
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            writeRGBA(fn(detail::readPixel(src, x, y)), dst, x, y);
}

/// Row-parallel mapPixels. Splits rows across the default thread pool when the
/// policy requests it (serial otherwise).
template <class Fn>
void mapPixels(const ConstImageView& src, ImageView dst, Fn&& fn,
               const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid()) throw InvalidParameterError("proc::mapPixels: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height())
        throw InvalidParameterError("proc::mapPixels: dimension mismatch");
    const uint32 w = src.width(), h = src.height();
    if (!wantsParallel(policy, static_cast<size_t>(w) * h, 4096)) {
        mapPixels(src, dst, static_cast<Fn&&>(fn));
        return;
    }
    const uint32 width = w;
    parallelForRows(h, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x)
            writeRGBA(fn(detail::readPixel(src, x, y)), dst, x, y);
    }, policy);
}

} // namespace detail
} // namespace proc
} // namespace iml