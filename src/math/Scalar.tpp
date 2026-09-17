#ifndef IMAGELIB_MATH_SCALAR_H_
#error "Include imagelib/math/Scalar.h, not this .tpp directly."
#endif

namespace iml {
namespace math {

/// Fractional part in [0, 1).
template <typename T> T fract(T x) noexcept {
    T f = x - floor_(x);
    return f;
}

/// Returns a mod b with the result in [0, b).
/// @tparam T Floating-point type.
/// @param a Dividend.
/// @param b Divisor.
/// @return The remainder in [0, b).
template <typename T> T mod(T a, T b) noexcept {
    T r = std::fmod(a, b);
    return r < T(0) ? r + b : r;
}

/// Returns the reciprocal square root of x.
/// @tparam T Floating-point type.
/// @param x Input value.
/// @return 1 / sqrt(x).
template <typename T> T rsqrt(T x) noexcept { return T(1) / sqrt_(x); }

/// Returns the normalized position of v within [a, b].
/// @tparam T Floating-point type.
/// @param a Lower bound.
/// @param b Upper bound.
/// @param v Value to locate.
/// @return 0 when a == b, else (v - a) / (b - a).
template <typename T> T inverseLerp(T a, T b, T v) noexcept {
    return (b != a) ? (v - a) / (b - a) : T(0);
}

/// Maps v from [a, b] into [c, d].
template <typename T> T remap(T v, T a, T b, T c, T d) noexcept {
    return lerp(c, d, inverseLerp(a, b, v));
}

} // namespace math
} // namespace iml