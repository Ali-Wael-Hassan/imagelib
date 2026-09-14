#pragma once
#define IMAGELIB_MATH_SCALAR_H_
// imagelib/math/Scalar.h
//
// Reusable scalar math: constants + common functions. Deliberately free of
// any image-system dependency so it can be used everywhere (Math, Processing,
// Procedural, SIMD scalar fallback).
//
// constexpr templates live here; non-constexpr template bodies live in
// Scalar.tpp; non-template scalar wrappers are compiled out-of-line in
// src/math/Scalar.cpp.

#include "imagelib/core/Types.h"

#include <cmath>
#include <algorithm>
#include <type_traits>
#include <limits>

namespace iml {
namespace math {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

constexpr double pi           = 3.14159265358979323846;
constexpr double twoPi        = 6.28318530717958647692;
constexpr double halfPi       = 1.57079632679489661923;
constexpr double quarterPi    = 0.78539816339744830962;
constexpr double oneOverPi    = 0.31830988618379067154;
constexpr double e            = 2.71828182845904523536;
constexpr double ln2          = 0.69314718055994530942;
constexpr double sqrt2        = 1.41421356237309504880;
constexpr double oneOverSqrt2 = 0.70710678118654752440;

constexpr float piF           = static_cast<float>(pi);
constexpr float twoPiF        = static_cast<float>(twoPi);
constexpr float halfPiF       = static_cast<float>(halfPi);

/// Bytes per single channel: degrees -> radians (templated on float/double).
template <typename T>
constexpr T toRadians(T degrees) noexcept { return degrees * static_cast<T>(pi) / static_cast<T>(180); }
template <typename T>
constexpr T toDegrees(T radians) noexcept { return radians * static_cast<T>(180) / static_cast<T>(pi); }

// ---------------------------------------------------------------------------
// Common functions
// ---------------------------------------------------------------------------

template <typename T>
constexpr T absVal(T x) noexcept { return x < T(0) ? -x : x; }

template <typename T>
constexpr T minVal(T a, T b) noexcept { return a < b ? a : b; }

template <typename T>
constexpr T maxVal(T a, T b) noexcept { return a > b ? a : b; }

template <typename T>
constexpr T clamp(T x, T lo, T hi) noexcept { return x < lo ? lo : (x > hi ? hi : x); }

/// Clamps to [0, 1] for floating point types.
template <typename T, typename = std::enable_if_t<std::is_floating_point<T>::value>>
constexpr T saturate(T x) noexcept { return clamp(x, T(0), T(1)); }

/// Sign of x: -1, 0 or +1.
template <typename T>
constexpr T sign(T x) noexcept { return x < T(0) ? T(-1) : (x > T(0) ? T(1) : T(0)); }

float floor_(float x)  noexcept;
double floor_(double x) noexcept;
float ceil_(float x)   noexcept;
double ceil_(double x) noexcept;
float round_(float x)  noexcept;
double round_(double x) noexcept;

/// Fractional part in [0, 1).
template <typename T>
T fract(T x) noexcept;

template <typename T>
T mod(T a, T b) noexcept;

float sqrt_(float x)  noexcept;
double sqrt_(double x) noexcept;
template <typename T>
T rsqrt(T x) noexcept;

float pow_(float base, float exp_) noexcept;
double pow_(double base, double exp_) noexcept;
float exp_(float x) noexcept;
double exp_(double x) noexcept;
float log_(float x) noexcept;
double log_(double x) noexcept;

float sin_(float x) noexcept;
double sin_(double x) noexcept;
float cos_(float x) noexcept;
double cos_(double x) noexcept;
float tan_(float x) noexcept;
double tan_(double x) noexcept;
float asin_(float x) noexcept;
double asin_(double x) noexcept;
float acos_(float x) noexcept;
double acos_(double x) noexcept;
float atan_(float x) noexcept;
double atan_(double x) noexcept;
float atan2_(float y, float x) noexcept;
double atan2_(double y, double x) noexcept;

// ---------------------------------------------------------------------------
// Interpolation primitives
// ---------------------------------------------------------------------------

template <typename T>
constexpr T lerp(T a, T b, T t) noexcept { return a + (b - a) * t; }

template <typename T>
T inverseLerp(T a, T b, T v) noexcept;

/// Maps v from [a, b] into [c, d].
template <typename T>
T remap(T v, T a, T b, T c, T d) noexcept;

/// Hermite smoothstep basis in [0,1].
template <typename T>
constexpr T smoothstep(T e0, T e1, T x) noexcept {
    T t = clamp((x - e0) / (e1 - e0), T(0), T(1));
    return t * t * (T(3) - T(2) * t);
}

template <typename T>
constexpr T smootherstep(T e0, T e1, T x) noexcept {
    T t = clamp((x - e0) / (e1 - e0), T(0), T(1));
    return t * t * t * (t * (t * T(6) - T(15)) + T(10));
}

// ---------------------------------------------------------------------------
// Bit helpers
// ---------------------------------------------------------------------------

constexpr bool isPowerOfTwo(uint32 v) noexcept { return v != 0 && (v & (v - 1)) == 0; }

constexpr uint32 nextPowerOfTwo(uint32 v) noexcept {
    --v;
    v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
    return v + 1;
}

} // namespace math
} // namespace iml

#include "imagelib/math/Scalar.tpp"