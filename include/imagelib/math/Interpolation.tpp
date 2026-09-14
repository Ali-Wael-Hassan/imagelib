#ifndef IMAGELIB_MATH_INTERPOLATION_H_
#error "Include imagelib/math/Interpolation.h, not this .tpp directly."
#endif

namespace iml {
namespace math {

/// Trilinear interpolation of 8 cube corners (indices: (x,y,z) in {0,1}^3).
template <typename T>
T trilinear(const std::array<T, 8>& c, const std::array<T, 3>& t) noexcept {
    return lerp(
        lerp(lerp(c[0], c[1], t[0]), lerp(c[2], c[3], t[0]), t[1]),
        lerp(lerp(c[4], c[5], t[0]), lerp(c[6], c[7], t[0]), t[1]),
        t[2]);
}

// -- cubic families --------------------------------------------------------

/// Cubic Hermite spline: f(0)=p0, f'(0)=m0, f(1)=p1, f'(1)=m1.
template <typename T>
T hermite(T p0, T p1, T m0, T m1, T t) noexcept {
    T t2 = t * t;
    T t3 = t2 * t;
    return (2*t3 - 3*t2 + 1) * p0 + (t3 - 2*t2 + t) * m0
         + (-2*t3 + 3*t2)     * p1 + (t3 - t2)        * m1;
}

/// Catmull-Rom through p1/p2 with tangent neighbors p0/p3.
template <typename T>
T catmullRom(T p0, T p1, T p2, T p3, T t) noexcept {
    T t2 = t * t;
    T t3 = t2 * t;
    return T(0.5) * ((2*p1) + (-p0 + p2) * t + (2*p0 - 5*p1 + 4*p2 - p3) * t2
                     + (-p0 + 3*p1 - 3*p2 + p3) * t3);
}

template <typename T>
T quadraticBezier(T p0, T p1, T p2, T t) noexcept {
    T s = T(1) - t;
    return s*s*p0 + 2*s*t*p1 + t*t*p2;
}

template <typename T>
T cubicBezier(T p0, T p1, T p2, T p3, T t) noexcept {
    T s = T(1) - t;
    return s*s*s*p0 + 3*s*s*t*p1 + 3*s*t*t*p2 + t*t*t*p3;
}

// -- bicubic ----------------------------------------------------------------

/// Bicubic interpolation of a 4x4 patch using cubic Catmull-Rom along each
/// row then across columns. `tap[i*4+j]` at (j, i) (column-first ordering
/// matching how row-major patches are commonly stored).
template <typename T>
T bicubicCatmullRom(const T* tap16, T tx, T ty) noexcept {
    std::array<T, 4> rows;
    for (int r = 0; r < 4; ++r) {
        rows[static_cast<size_t>(r)] = catmullRom(
            tap16[r * 4 + 0], tap16[r * 4 + 1],
            tap16[r * 4 + 2], tap16[r * 4 + 3], tx);
    }
    return catmullRom(rows[0], rows[1], rows[2], rows[3], ty);
}

/// Cubic convolution kernel weights for `x` in [-2, 2] (Lanczos-3 / Catmull-Rom
/// style, a = -0.5 for Catmull-Rom, a = -1 for Mitchell-Netravali-ish).
template <typename T>
T cubicKernel(T x, T a = T(-0.5)) noexcept {
    x = std::abs(x);
    T x2 = x * x, x3 = x2 * x;
    if (x < 1)      return (a + 2) * x3 - (a + 3) * x2 + 1;
    else if (x < 2) return a * x3 - 5 * a * x2 + 8 * a * x - 4 * a;
    else            return 0;
}

/// Lanczos kernel (a taps).
template <typename T>
T lanczosKernel(T x, int a) noexcept {
    if (x == 0) return 1;
    if (std::abs(x) >= a) return 0;
    T p = pi * x;
    return a * std::sin(p) * std::sin(p / a) / (p * p);
}

} // namespace math
} // namespace iml