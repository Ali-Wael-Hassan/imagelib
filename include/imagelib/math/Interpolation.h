#pragma once
#define IMAGELIB_MATH_INTERPOLATION_H_
/// @file
/// Interpolation primitives reusable by resize, procedural generation and
/// noise. Pure scalar math (float/double), no image dependency.

#include "imagelib/core/Types.h"
#include "imagelib/math/Scalar.h"

#include <cmath>
#include <array>

namespace iml {
namespace math {

/// Bilinear interpolation over the unit square.
/// @tparam T scalar type.
/// @param p00 Value at (0, 0).
/// @param p10 Value at (1, 0).
/// @param p01 Value at (0, 1).
/// @param p11 Value at (1, 1).
/// @param tx X interpolation parameter in [0, 1].
/// @param ty Y interpolation parameter in [0, 1].
/// @return Interpolated value.
template <typename T> constexpr T bilinear(T p00, T p10, T p01, T p11, T tx, T ty) noexcept {
    T top = lerp(p00, p10, tx);
    T bottom = lerp(p01, p11, tx);
    return lerp(top, bottom, ty);
}

} // namespace math
} // namespace iml

#include "math/Interpolation.tpp"