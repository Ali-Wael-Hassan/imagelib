#pragma once
#define IMAGELIB_MATH_VECTOR_H_
// imagelib/math/Vector.h
//
// Lightweight vector types: Vec2/Vec3/Vec4 (float), IVec (int32), UVec
// (uint32) and CVec (uint8). Trivially copyable, SIMD-friendly, no heap
// allocation. Standard layout so they can be reinterpret-cast for SIMD use.
//
// constexpr vector math lives here; non-constexpr template bodies (members of
// the class templates and the free-function vector math) live in Vector.tpp.

#include "imagelib/core/Types.h"
#include "imagelib/math/Scalar.h"

#include <cmath>
#include <initializer_list>

namespace iml {
namespace math {

// ---------------------------------------------------------------------------
// TVec2
// ---------------------------------------------------------------------------
template <typename T>
struct TVec2 {
    T x = T(0);
    T y = T(0);

    constexpr TVec2() noexcept = default;
    constexpr TVec2(T xi, T yi) noexcept : x(xi), y(yi) {}
    explicit constexpr TVec2(T v) noexcept : x(v), y(v) {}

    static constexpr TVec2 zero() noexcept { return TVec2(T(0), T(0)); }
    static constexpr TVec2 one()  noexcept { return TVec2(T(1), T(1)); }

    constexpr T operator[](int i) const noexcept { return i == 0 ? x : y; }
    T& operator[](int i) noexcept;

    constexpr TVec2 operator-() const noexcept { return {T(-x), T(-y)}; }

    TVec2& operator+=(const TVec2& o) noexcept;
    TVec2& operator-=(const TVec2& o) noexcept;
    TVec2& operator*=(const TVec2& o) noexcept;
    TVec2& operator/=(const TVec2& o) noexcept;
    TVec2& operator*=(T s) noexcept;
    TVec2& operator/=(T s) noexcept;

    // -- geometry ----------------------------------------------------------
    T squaredLength() const noexcept;
    T length() const noexcept;

    TVec2& normalize() noexcept;
    TVec2 normalized() const noexcept;

    constexpr T dot(const TVec2& o) const noexcept { return x * o.x + y * o.y; }

    T distance(const TVec2& o) const noexcept;
    T squaredDistance(const TVec2& o) const noexcept;

    constexpr bool operator==(const TVec2& o) const noexcept { return x == o.x && y == o.y; }
    constexpr bool operator!=(const TVec2& o) const noexcept { return !(*this == o); }
};

template <typename T>
constexpr TVec2<T> operator+(const TVec2<T>& a, const TVec2<T>& b) noexcept { TVec2<T> r(a); r += b; return r; }
template <typename T>
constexpr TVec2<T> operator-(const TVec2<T>& a, const TVec2<T>& b) noexcept { TVec2<T> r(a); r -= b; return r; }
template <typename T>
constexpr TVec2<T> operator*(const TVec2<T>& a, const TVec2<T>& b) noexcept { TVec2<T> r(a); r *= b; return r; }
template <typename T>
constexpr TVec2<T> operator/(const TVec2<T>& a, const TVec2<T>& b) noexcept { TVec2<T> r(a); r /= b; return r; }
template <typename T>
constexpr TVec2<T> operator*(const TVec2<T>& a, T s) noexcept { TVec2<T> r(a); r *= s; return r; }
template <typename T>
constexpr TVec2<T> operator*(T s, const TVec2<T>& a) noexcept { return a * s; }
template <typename T>
constexpr TVec2<T> operator/(const TVec2<T>& a, T s) noexcept { TVec2<T> r(a); r /= s; return r; }

// ---------------------------------------------------------------------------
// TVec3
// ---------------------------------------------------------------------------
template <typename T>
struct TVec3 {
    T x = T(0);
    T y = T(0);
    T z = T(0);

    constexpr TVec3() noexcept = default;
    constexpr TVec3(T xi, T yi, T zi) noexcept : x(xi), y(yi), z(zi) {}
    constexpr explicit TVec3(const TVec2<T>& v, T zi = T(0)) noexcept : x(v.x), y(v.y), z(zi) {}
    explicit constexpr TVec3(T v) noexcept : x(v), y(v), z(v) {}

    static constexpr TVec3 zero() noexcept { return TVec3(T(0), T(0), T(0)); }
    static constexpr TVec3 one()  noexcept { return TVec3(T(1), T(1), T(1)); }

    constexpr T operator[](int i) const noexcept { return i == 0 ? x : (i == 1 ? y : z); }
    T& operator[](int i) noexcept;

    constexpr TVec3 operator-() const noexcept { return {T(-x), T(-y), T(-z)}; }

    TVec3& operator+=(const TVec3& o) noexcept;
    TVec3& operator-=(const TVec3& o) noexcept;
    TVec3& operator*=(const TVec3& o) noexcept;
    TVec3& operator/=(const TVec3& o) noexcept;
    TVec3& operator*=(T s) noexcept;
    TVec3& operator/=(T s) noexcept;

    T squaredLength() const noexcept;
    T length() const noexcept;

    TVec3& normalize() noexcept;
    TVec3 normalized() const noexcept;

    constexpr T dot(const TVec3& o) const noexcept { return x * o.x + y * o.y + z * o.z; }
    constexpr TVec3 cross(const TVec3& o) const noexcept {
        return { y * o.z - z * o.y,
                 z * o.x - x * o.z,
                 x * o.y - y * o.x };
    }

    T distance(const TVec3& o) const noexcept;
    T squaredDistance(const TVec3& o) const noexcept;

    constexpr bool operator==(const TVec3& o) const noexcept { return x == o.x && y == o.y && z == o.z; }
    constexpr bool operator!=(const TVec3& o) const noexcept { return !(*this == o); }
};

template <typename T>
constexpr TVec3<T> operator+(const TVec3<T>& a, const TVec3<T>& b) noexcept { TVec3<T> r(a); r += b; return r; }
template <typename T>
constexpr TVec3<T> operator-(const TVec3<T>& a, const TVec3<T>& b) noexcept { TVec3<T> r(a); r -= b; return r; }
template <typename T>
constexpr TVec3<T> operator*(const TVec3<T>& a, const TVec3<T>& b) noexcept { TVec3<T> r(a); r *= b; return r; }
template <typename T>
constexpr TVec3<T> operator/(const TVec3<T>& a, const TVec3<T>& b) noexcept { TVec3<T> r(a); r /= b; return r; }
template <typename T>
constexpr TVec3<T> operator*(const TVec3<T>& a, T s) noexcept { TVec3<T> r(a); r *= s; return r; }
template <typename T>
constexpr TVec3<T> operator*(T s, const TVec3<T>& a) noexcept { return a * s; }
template <typename T>
constexpr TVec3<T> operator/(const TVec3<T>& a, T s) noexcept { TVec3<T> r(a); r /= s; return r; }

// ---------------------------------------------------------------------------
// TVec4
// ---------------------------------------------------------------------------
template <typename T>
struct TVec4 {
    T x = T(0);
    T y = T(0);
    T z = T(0);
    T w = T(0);

    constexpr TVec4() noexcept = default;
    constexpr TVec4(T xi, T yi, T zi, T wi) noexcept : x(xi), y(yi), z(zi), w(wi) {}
    constexpr explicit TVec4(const TVec3<T>& v, T wi = T(0)) noexcept : x(v.x), y(v.y), z(v.z), w(wi) {}
    explicit constexpr TVec4(T v) noexcept : x(v), y(v), z(v), w(v) {}

    static constexpr TVec4 zero() noexcept { return TVec4(T(0), T(0), T(0), T(0)); }
    static constexpr TVec4 one()  noexcept { return TVec4(T(1), T(1), T(1), T(1)); }

    constexpr T operator[](int i) const noexcept { return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w)); }
    T& operator[](int i) noexcept;

    constexpr TVec4 operator-() const noexcept { return {T(-x), T(-y), T(-z), T(-w)}; }

    TVec4& operator+=(const TVec4& o) noexcept;
    TVec4& operator-=(const TVec4& o) noexcept;
    TVec4& operator*=(const TVec4& o) noexcept;
    TVec4& operator/=(const TVec4& o) noexcept;
    TVec4& operator*=(T s) noexcept;
    TVec4& operator/=(T s) noexcept;

    T squaredLength() const noexcept;
    T length() const noexcept;

    TVec4& normalize() noexcept;
    TVec4 normalized() const noexcept;

    constexpr T dot(const TVec4& o) const noexcept { return x * o.x + y * o.y + z * o.z + w * o.w; }

    T distance(const TVec4& o) const noexcept;
    T squaredDistance(const TVec4& o) const noexcept;

    constexpr bool operator==(const TVec4& o) const noexcept {
        return x == o.x && y == o.y && z == o.z && w == o.w;
    }
    constexpr bool operator!=(const TVec4& o) const noexcept { return !(*this == o); }
};

template <typename T>
constexpr TVec4<T> operator+(const TVec4<T>& a, const TVec4<T>& b) noexcept { TVec4<T> r(a); r += b; return r; }
template <typename T>
constexpr TVec4<T> operator-(const TVec4<T>& a, const TVec4<T>& b) noexcept { TVec4<T> r(a); r -= b; return r; }
template <typename T>
constexpr TVec4<T> operator*(const TVec4<T>& a, const TVec4<T>& b) noexcept { TVec4<T> r(a); r *= b; return r; }
template <typename T>
constexpr TVec4<T> operator/(const TVec4<T>& a, const TVec4<T>& b) noexcept { TVec4<T> r(a); r /= b; return r; }
template <typename T>
constexpr TVec4<T> operator*(const TVec4<T>& a, T s) noexcept { TVec4<T> r(a); r *= s; return r; }
template <typename T>
constexpr TVec4<T> operator*(T s, const TVec4<T>& a) noexcept { TVec4<T> r(a); r *= s; return r; }
template <typename T>
constexpr TVec4<T> operator/(const TVec4<T>& a, T s) noexcept { TVec4<T> r(a); r /= s; return r; }

// ---------------------------------------------------------------------------
// Aliases
// ---------------------------------------------------------------------------
using Vec2 = TVec2<float>;
using Vec3 = TVec3<float>;
using Vec4 = TVec4<float>;
using DVec2 = TVec2<double>;
using DVec3 = TVec3<double>;
using DVec4 = TVec4<double>;
using IVec2 = TVec2<int32>;
using IVec3 = TVec3<int32>;
using IVec4 = TVec4<int32>;
using UVec2 = TVec2<uint32>;
using UVec3 = TVec3<uint32>;
using UVec4 = TVec4<uint32>;
using CVec2 = TVec2<uint8>;
using CVec3 = TVec3<uint8>;
using CVec4 = TVec4<uint8>;

// ---------------------------------------------------------------------------
// Free-function vector math
// ---------------------------------------------------------------------------

namespace detail {
template <typename T>
constexpr bool isScalar() { return std::is_floating_point<T>::value || std::is_integral<T>::value; }
}

template <typename T>
constexpr TVec2<T> minElementWise(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    return { minVal(a.x, b.x), minVal(a.y, b.y) };
}
template <typename T>
constexpr TVec3<T> minElementWise(const TVec3<T>& a, const TVec3<T>& b) noexcept {
    return { minVal(a.x, b.x), minVal(a.y, b.y), minVal(a.z, b.z) };
}
template <typename T>
constexpr TVec4<T> minElementWise(const TVec4<T>& a, const TVec4<T>& b) noexcept {
    return { minVal(a.x, b.x), minVal(a.y, b.y), minVal(a.z, b.z), minVal(a.w, b.w) };
}
template <typename T>
constexpr TVec2<T> maxElementWise(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    return { maxVal(a.x, b.x), maxVal(a.y, b.y) };
}
template <typename T>
constexpr TVec3<T> maxElementWise(const TVec3<T>& a, const TVec3<T>& b) noexcept {
    return { maxVal(a.x, b.x), maxVal(a.y, b.y), maxVal(a.z, b.z) };
}
template <typename T>
constexpr TVec4<T> maxElementWise(const TVec4<T>& a, const TVec4<T>& b) noexcept {
    return { maxVal(a.x, b.x), maxVal(a.y, b.y), maxVal(a.z, b.z), maxVal(a.w, b.w) };
}

template <typename T>
constexpr TVec2<T> clampComponentwise(const TVec2<T>& v, const TVec2<T>& lo, const TVec2<T>& hi) noexcept {
    return { clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y) };
}
template <typename T>
constexpr TVec3<T> clampComponentwise(const TVec3<T>& v, const TVec3<T>& lo, const TVec3<T>& hi) noexcept {
    return { clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z) };
}
template <typename T>
constexpr TVec4<T> clampComponentwise(const TVec4<T>& v, const TVec4<T>& lo, const TVec4<T>& hi) noexcept {
    return { clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z), clamp(v.w, lo.w, hi.w) };
}

/// Element-wise abs.
template <typename T> constexpr TVec2<T> absVec(const TVec2<T>& v) noexcept { return { absVal(v.x), absVal(v.y) }; }
template <typename T> constexpr TVec3<T> absVec(const TVec3<T>& v) noexcept { return { absVal(v.x), absVal(v.y), absVal(v.z) }; }
template <typename T> constexpr TVec4<T> absVec(const TVec4<T>& v) noexcept { return { absVal(v.x), absVal(v.y), absVal(v.z), absVal(v.w) }; }

template <typename T>
constexpr T dot(const TVec2<T>& a, const TVec2<T>& b) noexcept { return a.dot(b); }
template <typename T>
constexpr T dot(const TVec3<T>& a, const TVec3<T>& b) noexcept { return a.dot(b); }
template <typename T>
constexpr T dot(const TVec4<T>& a, const TVec4<T>& b) noexcept { return a.dot(b); }

template <typename T>
constexpr TVec3<T> cross(const TVec3<T>& a, const TVec3<T>& b) noexcept { return a.cross(b); }

// 2D cross returns the scalar z of the cross product of the 3D extensions.
template <typename T>
constexpr T cross(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    return a.x * b.y - a.y * b.x;
}

template <typename T>
T length(const TVec2<T>& v) noexcept;
template <typename T>
T length(const TVec3<T>& v) noexcept;
template <typename T>
T length(const TVec4<T>& v) noexcept;

template <typename T>
TVec2<T> normalize(const TVec2<T>& v) noexcept;
template <typename T>
TVec3<T> normalize(const TVec3<T>& v) noexcept;
template <typename T>
TVec4<T> normalize(const TVec4<T>& v) noexcept;

template <typename T>
T distance(const TVec2<T>& a, const TVec2<T>& b) noexcept;
template <typename T>
T distance(const TVec3<T>& a, const TVec3<T>& b) noexcept;

/// Reflects v about the unit normal n: v - 2*(v.n)*n.
template <typename T>
TVec2<T> reflect(const TVec2<T>& v, const TVec2<T>& n) noexcept;
template <typename T>
TVec3<T> reflect(const TVec3<T>& v, const TVec3<T>& n) noexcept;

/// Projects v onto the direction d (d need not be normalized).
template <typename T>
TVec2<T> project(const TVec2<T>& v, const TVec2<T>& d) noexcept;
template <typename T>
TVec3<T> project(const TVec3<T>& v, const TVec3<T>& d) noexcept;

} // namespace math
} // namespace iml

#include "imagelib/math/Vector.tpp"