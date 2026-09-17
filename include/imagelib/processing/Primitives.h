#pragma once
#define IMAGELIB_PROCESSING_PRIMITIVES_H_
/// @file Primitives.h
/// Building-block helpers for writing filters: the canonical per-pixel mapPixels
/// mapper plus the SIMD loop family, convolution kernels and color math.

#include "imagelib/core/image/Pixel.h"
#include "imagelib/core/ExecutionPolicy.h"
#include "imagelib/threading/ParallelFor.h"

#include <cstddef>

namespace iml {
namespace proc {

/// Expose the threading-threshold helper so kernels can gate on the policy.
using ::iml::detail::wantsParallel;

/// Loop over every pixel, mapping canonical RGBA pixels through `fn` (serial).
/// @tparam Fn Callable invoked with a canonical Pixel; result written as RGBA.
/// @param src Source image view.
/// @param dst Destination image view (same dimensions).
/// @param fn Per-pixel mapping callable.
/// @throws InvalidParameterError on invalid view or dimension mismatch.
template <class Fn> void mapPixels(const ConstImageView& src, ImageView dst, Fn&& fn) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("proc::mapPixels: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height())
        throw InvalidParameterError("proc::mapPixels: dimension mismatch");
    const uint32 w = src.width(), h = src.height();
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            pixel::writeRGBA(fn(pixel::readPixel(src, x, y)), dst, x, y);
}

/// Row-parallel mapPixels. Splits rows across the thread pool when the policy
/// requests it (serial otherwise).
/// @tparam Fn Callable invoked with a canonical Pixel; result written as RGBA.
/// @param src Source image view.
/// @param dst Destination image view (same dimensions).
/// @param fn Per-pixel mapping callable.
/// @param policy Execution policy governing parallelism.
/// @throws InvalidParameterError on invalid view or dimension mismatch.
template <class Fn>
void mapPixels(const ConstImageView& src, ImageView dst, Fn&& fn, const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("proc::mapPixels: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height())
        throw InvalidParameterError("proc::mapPixels: dimension mismatch");
    const uint32 w = src.width(), h = src.height();
    if (!wantsParallel(policy, static_cast<size_t>(w) * h, 4096)) {
        mapPixels(src, dst, static_cast<Fn&&>(fn));
        return;
    }
    const uint32 width = w;
    parallelForRows(
        h,
        [&](size_t row) {
            const uint32 y = static_cast<uint32>(row);
            for (uint32 x = 0; x < width; ++x)
                pixel::writeRGBA(fn(pixel::readPixel(src, x, y)), dst, x, y);
        },
        policy);
}

} // namespace proc
} // namespace iml