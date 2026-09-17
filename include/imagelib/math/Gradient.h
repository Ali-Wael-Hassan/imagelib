#pragma once
#define IMAGELIB_MATH_GRADIENT_H_
/// @file
/// 2D gradient of a scalar field by finite differences.

#include "imagelib/math/Vector.h"

#include <cmath>

namespace iml {
namespace math {

/// Central-difference gradient of a scalar field f at (x, y):
///   df/dx = (f(x+h, y) - f(x-h, y)) / (2h),  df/dy likewise.
/// Exact for linear fields; second-order accurate elsewhere. h defaults to one
/// grid step, so plugging an image sampler directly yields per-pixel
/// derivatives in sample units.
/// @tparam F Callable type `float F(float, float)`.
/// @param f Scalar field sampler.
/// @param x Sample x.
/// @param y Sample y.
/// @param h Step size.
/// @return Gradient vector.
template <class F>
inline TVec2<float> gradient2(const F& f, float x, float y, float h = 1.f) noexcept {
    return TVec2<float>(
        (f(x + h, y) - f(x - h, y)) / (2.f * h),
        (f(x, y + h) - f(x, y - h)) / (2.f * h));
}

/// Forward-difference gradient (one-sided; cheaper, first-order accurate).
/// @tparam F Callable type `float F(float, float)`.
/// @param f Scalar field sampler.
/// @param x Sample x.
/// @param y Sample y.
/// @param h Step size.
/// @return Gradient vector.
template <class F>
inline TVec2<float> gradient2Forward(const F& f, float x, float y, float h = 1.f) noexcept {
    return TVec2<float>((f(x + h, y) - f(x, y)) / h, (f(x, y + h) - f(x, y)) / h);
}

/// Gradient magnitude of a scalar field at (x, y) (central difference).
/// @tparam F Callable type `float F(float, float)`.
/// @param f Scalar field sampler.
/// @param x Sample x.
/// @param y Sample y.
/// @param h Step size.
/// @return Gradient magnitude.
template <class F>
inline float gradientMagnitude2(const F& f, float x, float y, float h = 1.f) noexcept {
    const TVec2<float> g = gradient2(f, x, y, h);
    return std::sqrt(g.x * g.x + g.y * g.y);
}

} // namespace math
} // namespace iml