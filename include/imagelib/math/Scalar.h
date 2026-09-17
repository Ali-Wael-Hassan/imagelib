#pragma once
#define IMAGELIB_MATH_SCALAR_H_
/// @file
/// Reusable scalar math: constants and common functions, free of image-system
/// dependencies.

#include "imagelib/core/Types.h"

#include <cmath>
#include <algorithm>
#include <type_traits>
#include <limits>

namespace iml {
namespace math {

/// Pi constant.
constexpr double pi = 3.14159265358979323846;
/// Twice pi.
constexpr double twoPi = 6.28318530717958647692;
/// Half pi.
constexpr double halfPi = 1.57079632679489661923;
/// Quarter pi.
constexpr double quarterPi = 0.78539816339744830962;
/// Reciprocal of pi.
constexpr double oneOverPi = 0.31830988618379067154;
/// Euler's number.
constexpr double e = 2.71828182845904523536;
/// Natural logarithm of 2.
constexpr double ln2 = 0.69314718055994530942;
/// Square root of 2.
constexpr double sqrt2 = 1.41421356237309504880;
/// Reciprocal of the square root of 2.
constexpr double oneOverSqrt2 = 0.70710678118654752440;

/// Pi as float.
constexpr float piF = static_cast<float>(pi);
/// Twice pi as float.
constexpr float twoPiF = static_cast<float>(twoPi);
/// Half pi as float.
constexpr float halfPiF = static_cast<float>(halfPi);

/// Converts degrees to radians.
/// @tparam T scalar type.
/// @param degrees Angle in degrees.
/// @return Angle in radians.
template <typename T> constexpr T toRadians(T degrees) noexcept {
    return degrees * static_cast<T>(pi) / static_cast<T>(180);
}
/// Converts radians to degrees.
/// @tparam T scalar type.
/// @param radians Angle in radians.
/// @return Angle in degrees.
template <typename T> constexpr T toDegrees(T radians) noexcept {
    return radians * static_cast<T>(180) / static_cast<T>(pi);
}

/// Returns the absolute value of x.
/// @tparam T scalar type.
/// @param x Value to take the absolute value of.
/// @return Absolute value of x.
template <typename T> constexpr T absVal(T x) noexcept { return x < T(0) ? -x : x; }

/// Returns the smaller of a and b.
/// @tparam T scalar type.
/// @param a First value.
/// @param b Second value.
/// @return Minimum of a and b.
template <typename T> constexpr T minVal(T a, T b) noexcept { return a < b ? a : b; }

/// Returns the larger of a and b.
/// @tparam T scalar type.
/// @param a First value.
/// @param b Second value.
/// @return Maximum of a and b.
template <typename T> constexpr T maxVal(T a, T b) noexcept { return a > b ? a : b; }

/// Clamps x to the range [lo, hi].
/// @tparam T scalar type.
/// @param x Value to clamp.
/// @param lo Lower bound.
/// @param hi Upper bound.
/// @return Clamped value.
template <typename T> constexpr T clamp(T x, T lo, T hi) noexcept {
    return x < lo ? lo : (x > hi ? hi : x);
}

/// Clamps x to [0, 1].
/// @tparam T scalar type.
/// @param x Value to clamp.
/// @return Value clamped to [0, 1].
template <typename T> constexpr T clamp01(T x) noexcept { return clamp(x, T(0), T(1)); }

/// Clamps to [0, 1] for floating point types.
/// @tparam T floating point type.
/// @param x Value to clamp.
/// @return Value clamped to [0, 1].
template <typename T, typename = std::enable_if_t<std::is_floating_point<T>::value>>
constexpr T saturate(T x) noexcept {
    return clamp(x, T(0), T(1));
}

/// Sign of x: -1, 0 or +1.
/// @tparam T scalar type.
/// @param x Input value.
/// @return -1, 0 or +1.
template <typename T> constexpr T sign(T x) noexcept {
    return x < T(0) ? T(-1) : (x > T(0) ? T(1) : T(0));
}

/// Floors to the nearest integer.
/// @param x Input value.
/// @return Floor of x.
float floor_(float x) noexcept;
/// Floors to the nearest integer.
/// @param x Input value.
/// @return Floor of x.
double floor_(double x) noexcept;
/// Ceils to the nearest integer.
/// @param x Input value.
/// @return Ceiling of x.
float ceil_(float x) noexcept;
/// Ceils to the nearest integer.
/// @param x Input value.
/// @return Ceiling of x.
double ceil_(double x) noexcept;
/// Rounds to the nearest integer.
/// @param x Input value.
/// @return Nearest integer to x.
float round_(float x) noexcept;
/// Rounds to the nearest integer.
/// @param x Input value.
/// @return Nearest integer to x.
double round_(double x) noexcept;

/// Fractional part in [0, 1).
/// @tparam T scalar type.
/// @param x Input value.
/// @return Fractional part of x.
template <typename T> T fract(T x) noexcept;

/// Modulo returning the remainder of a / b.
/// @tparam T scalar type.
/// @param a Dividend.
/// @param b Divisor.
/// @return Remainder of a / b.
template <typename T> T mod(T a, T b) noexcept;

/// Square root.
/// @param x Input value.
/// @return Square root of x.
float sqrt_(float x) noexcept;
/// Square root.
/// @param x Input value.
/// @return Square root of x.
double sqrt_(double x) noexcept;
/// Reciprocal square root.
/// @tparam T scalar type.
/// @param x Input value.
/// @return 1 / sqrt(x).
template <typename T> T rsqrt(T x) noexcept;

/// Power function: base raised to the power exp_.
/// @param base Base value.
/// @param exp_ Exponent.
/// @return base^exp_.
float pow_(float base, float exp_) noexcept;
/// Power function: base raised to the power exp_.
/// @param base Base value.
/// @param exp_ Exponent.
/// @return base^exp_.
double pow_(double base, double exp_) noexcept;
/// Natural exponential.
/// @param x Exponent value.
/// @return e^x.
float exp_(float x) noexcept;
/// Natural exponential.
/// @param x Exponent value.
/// @return e^x.
double exp_(double x) noexcept;
/// Natural logarithm.
/// @param x Input value.
/// @return ln(x).
float log_(float x) noexcept;
/// Natural logarithm.
/// @param x Input value.
/// @return ln(x).
double log_(double x) noexcept;

/// Sine.
/// @param x Angle in radians.
/// @return sin(x).
float sin_(float x) noexcept;
/// Sine.
/// @param x Angle in radians.
/// @return sin(x).
double sin_(double x) noexcept;
/// Cosine.
/// @param x Angle in radians.
/// @return cos(x).
float cos_(float x) noexcept;
/// Cosine.
/// @param x Angle in radians.
/// @return cos(x).
double cos_(double x) noexcept;
/// Tangent.
/// @param x Angle in radians.
/// @return tan(x).
float tan_(float x) noexcept;
/// Tangent.
/// @param x Angle in radians.
/// @return tan(x).
double tan_(double x) noexcept;
/// Arc sine.
/// @param x Value in [-1, 1].
/// @return Angle in radians.
float asin_(float x) noexcept;
/// Arc sine.
/// @param x Value in [-1, 1].
/// @return Angle in radians.
double asin_(double x) noexcept;
/// Arc cosine.
/// @param x Value in [-1, 1].
/// @return Angle in radians.
float acos_(float x) noexcept;
/// Arc cosine.
/// @param x Value in [-1, 1].
/// @return Angle in radians.
double acos_(double x) noexcept;
/// Arc tangent.
/// @param x Tangent value.
/// @return Angle in radians.
float atan_(float x) noexcept;
/// Arc tangent.
/// @param x Tangent value.
/// @return Angle in radians.
double atan_(double x) noexcept;
/// Two-argument arc tangent.
/// @param y Y component.
/// @param x X component.
/// @return Angle in radians.
float atan2_(float y, float x) noexcept;
/// Two-argument arc tangent.
/// @param y Y component.
/// @param x X component.
/// @return Angle in radians.
double atan2_(double y, double x) noexcept;

/// Linear interpolation between a and b at parameter t.
/// @tparam T scalar type.
/// @param a Start value.
/// @param b End value.
/// @param t Interpolation parameter.
/// @return Interpolated value.
template <typename T> constexpr T lerp(T a, T b, T t) noexcept { return a + (b - a) * t; }

/// Inverts lerp: returns t in [0, 1] for v between a and b.
/// @tparam T scalar type.
/// @param a Start value.
/// @param b End value.
/// @param v Interpolated value.
/// @return Interpolation parameter.
template <typename T> T inverseLerp(T a, T b, T v) noexcept;

/// Maps v from [a, b] into [c, d].
/// @tparam T scalar type.
/// @param v Value to remap.
/// @param a Source range start.
/// @param b Source range end.
/// @param c Target range start.
/// @param d Target range end.
/// @return Remapped value.
template <typename T> T remap(T v, T a, T b, T c, T d) noexcept;

/// Hermite smoothstep basis in [0,1].
/// @tparam T scalar type.
/// @param e0 Lower edge.
/// @param e1 Upper edge.
/// @param x Input value.
/// @return Smoothly interpolated value.
template <typename T> constexpr T smoothstep(T e0, T e1, T x) noexcept {
    T t = clamp((x - e0) / (e1 - e0), T(0), T(1));
    return t * t * (T(3) - T(2) * t);
}

/// Quintic smootherstep basis in [0,1].
/// @tparam T scalar type.
/// @param e0 Lower edge.
/// @param e1 Upper edge.
/// @param x Input value.
/// @return Smoothly interpolated value.
template <typename T> constexpr T smootherstep(T e0, T e1, T x) noexcept {
    T t = clamp((x - e0) / (e1 - e0), T(0), T(1));
    return t * t * t * (t * (t * T(6) - T(15)) + T(10));
}

/// Returns true when v is an exact power of two.
/// @param v Value to test.
/// @return True when v is a power of two.
constexpr bool isPowerOfTwo(uint32 v) noexcept { return v != 0 && (v & (v - 1)) == 0; }

/// Rounds v up to the nearest power of two.
/// @param v Value to round up.
/// @return Next power of two at or above v.
constexpr uint32 nextPowerOfTwo(uint32 v) noexcept {
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}

} // namespace math
} // namespace iml

#include "math/Scalar.tpp"