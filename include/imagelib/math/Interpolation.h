#pragma once
#define IMAGELIB_MATH_INTERPOLATION_H_
// imagelib/math/Interpolation.h
//
// Interpolation primitives reusable by resize, procedural generation and
// noise: lerp, bilinear/trilinear/bicubic, Hermite, Catmull-Rom, Bezier,
// smoothstep. Pure scalar math (float/double) - no image dependency.
//
// constexpr primitives live here; non-constexpr template bodies live in
// Interpolation.tpp.

#include "imagelib/core/Types.h"
#include "imagelib/math/Scalar.h"

#include <cmath>
#include <array>

namespace iml {
namespace math {

// lerp / inverseLerp / smoothstep / smootherstep live in Scalar.h.

// -- bilinear / trilinear ------------------------------------------------
template <typename T>
constexpr T bilinear(T p00, T p10, T p01, T p11, T tx, T ty) noexcept {
    T top    = lerp(p00, p10, tx);
    T bottom = lerp(p01, p11, tx);
    return lerp(top, bottom, ty);
}

} // namespace math
} // namespace iml

#include "imagelib/math/Interpolation.tpp"