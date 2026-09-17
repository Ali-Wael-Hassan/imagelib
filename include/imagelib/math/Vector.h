#pragma once
#define IMAGELIB_MATH_VECTOR_H_
/// @file
/// Lightweight vector types: Vec2/Vec3/Vec4 (float), IVec (int32), UVec
/// (uint32) and CVec (uint8); trivially copyable and SIMD-friendly.

#include "imagelib/core/Types.h"
#include "imagelib/math/Scalar.h"

#include <cmath>
#include <initializer_list>

namespace iml {
namespace math {

/// Two-component vector with component-wise arithmetic.
template <typename T> struct TVec2 {
    /// X component.
    T x = T(0);
    /// Y component.
    T y = T(0);

    /// Default constructor; zero-initializes all components.
    constexpr TVec2() noexcept = default;
    /// Constructs from X and Y components.
    /// @param xi X component.
    /// @param yi Y component.
    constexpr TVec2(T xi, T yi) noexcept : x(xi), y(yi) {}
    /// Constructs a vector with every component set to v.
    /// @param v Value for each component.
    explicit constexpr TVec2(T v) noexcept : x(v), y(v) {}

    /// Returns the zero vector.
    /// @return TVec2(0, 0).
    static constexpr TVec2 zero() noexcept { return TVec2(T(0), T(0)); }
    /// Returns the one vector.
    /// @return TVec2(1, 1).
    static constexpr TVec2 one() noexcept { return TVec2(T(1), T(1)); }

    /// Returns the component at index i (0 = x, 1 = y).
    /// @param i Component index.
    /// @return Component value.
    constexpr T operator[](int i) const noexcept { return i == 0 ? x : y; }
    /// Returns a reference to the component at index i.
    /// @param i Component index.
    /// @return Reference to the component.
    T& operator[](int i) noexcept;

    /// Returns the negated vector.
    /// @return Component-wise negation of this.
    constexpr TVec2 operator-() const noexcept { return {T(-x), T(-y)}; }

    /// Adds o component-wise.
    /// @param o Vector to add.
    /// @return Reference to this.
    TVec2& operator+=(const TVec2& o) noexcept;
    /// Subtracts o component-wise.
    /// @param o Vector to subtract.
    /// @return Reference to this.
    TVec2& operator-=(const TVec2& o) noexcept;
    /// Multiplies component-wise by o.
    /// @param o Vector to multiply by.
    /// @return Reference to this.
    TVec2& operator*=(const TVec2& o) noexcept;
    /// Divides component-wise by o.
    /// @param o Vector to divide by.
    /// @return Reference to this.
    TVec2& operator/=(const TVec2& o) noexcept;
    /// Multiplies every component by scalar s.
    /// @param s Scalar factor.
    /// @return Reference to this.
    TVec2& operator*=(T s) noexcept;
    /// Divides every component by scalar s.
    /// @param s Scalar divisor.
    /// @return Reference to this.
    TVec2& operator/=(T s) noexcept;

    /// Returns the squared length.
    /// @return Squared magnitude.
    T squaredLength() const noexcept;
    /// Returns the length.
    /// @return Magnitude.
    T length() const noexcept;

    /// Normalizes this in place.
    /// @return Reference to this.
    TVec2& normalize() noexcept;
    /// Returns a normalized copy.
    /// @return Unit vector in the direction of this.
    TVec2 normalized() const noexcept;

    /// Returns the dot product with o.
    /// @param o Other vector.
    /// @return Dot product.
    constexpr T dot(const TVec2& o) const noexcept { return x * o.x + y * o.y; }

    /// Returns the distance to o.
    /// @param o Other vector.
    /// @return Euclidean distance.
    T distance(const TVec2& o) const noexcept;
    /// Returns the squared distance to o.
    /// @param o Other vector.
    /// @return Squared distance.
    T squaredDistance(const TVec2& o) const noexcept;

    /// Compares for component-wise equality.
    /// @param o Other vector.
    /// @return True when all components are equal.
    constexpr bool operator==(const TVec2& o) const noexcept { return x == o.x && y == o.y; }
    /// Compares for component-wise inequality.
    /// @param o Other vector.
    /// @return True when any component differs.
    constexpr bool operator!=(const TVec2& o) const noexcept { return !(*this == o); }
};

/// Adds a and b component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise sum.
template <typename T> constexpr TVec2<T> operator+(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    TVec2<T> r(a);
    r += b;
    return r;
}
/// Subtracts b from a component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise difference.
template <typename T> constexpr TVec2<T> operator-(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    TVec2<T> r(a);
    r -= b;
    return r;
}
/// Multiplies a and b component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise product.
template <typename T> constexpr TVec2<T> operator*(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    TVec2<T> r(a);
    r *= b;
    return r;
}
/// Divides a by b component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise quotient.
template <typename T> constexpr TVec2<T> operator/(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    TVec2<T> r(a);
    r /= b;
    return r;
}
/// Scales a by scalar s.
/// @tparam T component type.
/// @param a Vector to scale.
/// @param s Scalar factor.
/// @return Scaled vector.
template <typename T> constexpr TVec2<T> operator*(const TVec2<T>& a, T s) noexcept {
    TVec2<T> r(a);
    r *= s;
    return r;
}
/// Scales a by scalar s (commutative form).
/// @tparam T component type.
/// @param s Scalar factor.
/// @param a Vector to scale.
/// @return Scaled vector.
template <typename T> constexpr TVec2<T> operator*(T s, const TVec2<T>& a) noexcept {
    return a * s;
}
/// Divides a by scalar s.
/// @tparam T component type.
/// @param a Vector to divide.
/// @param s Scalar divisor.
/// @return Divided vector.
template <typename T> constexpr TVec2<T> operator/(const TVec2<T>& a, T s) noexcept {
    TVec2<T> r(a);
    r /= s;
    return r;
}

/// Three-component vector with component-wise arithmetic.
template <typename T> struct TVec3 {
    /// X component.
    T x = T(0);
    /// Y component.
    T y = T(0);
    /// Z component.
    T z = T(0);

    /// Default constructor; zero-initializes all components.
    constexpr TVec3() noexcept = default;
    /// Constructs from X, Y and Z components.
    /// @param xi X component.
    /// @param yi Y component.
    /// @param zi Z component.
    constexpr TVec3(T xi, T yi, T zi) noexcept : x(xi), y(yi), z(zi) {}
    /// Constructs from a TVec2 and an optional Z component.
    /// @param v X and Y components.
    /// @param zi Z component (defaults to 0).
    constexpr explicit TVec3(const TVec2<T>& v, T zi = T(0)) noexcept : x(v.x), y(v.y), z(zi) {}
    /// Constructs a vector with every component set to v.
    /// @param v Value for each component.
    explicit constexpr TVec3(T v) noexcept : x(v), y(v), z(v) {}

    /// Returns the zero vector.
    /// @return TVec3(0, 0, 0).
    static constexpr TVec3 zero() noexcept { return TVec3(T(0), T(0), T(0)); }
    /// Returns the one vector.
    /// @return TVec3(1, 1, 1).
    static constexpr TVec3 one() noexcept { return TVec3(T(1), T(1), T(1)); }

    /// Returns the component at index i (0 = x, 1 = y, 2 = z).
    /// @param i Component index.
    /// @return Component value.
    constexpr T operator[](int i) const noexcept { return i == 0 ? x : (i == 1 ? y : z); }
    /// Returns a reference to the component at index i.
    /// @param i Component index.
    /// @return Reference to the component.
    T& operator[](int i) noexcept;

    /// Returns the negated vector.
    /// @return Component-wise negation of this.
    constexpr TVec3 operator-() const noexcept { return {T(-x), T(-y), T(-z)}; }

    /// Adds o component-wise.
    /// @param o Vector to add.
    /// @return Reference to this.
    TVec3& operator+=(const TVec3& o) noexcept;
    /// Subtracts o component-wise.
    /// @param o Vector to subtract.
    /// @return Reference to this.
    TVec3& operator-=(const TVec3& o) noexcept;
    /// Multiplies component-wise by o.
    /// @param o Vector to multiply by.
    /// @return Reference to this.
    TVec3& operator*=(const TVec3& o) noexcept;
    /// Divides component-wise by o.
    /// @param o Vector to divide by.
    /// @return Reference to this.
    TVec3& operator/=(const TVec3& o) noexcept;
    /// Multiplies every component by scalar s.
    /// @param s Scalar factor.
    /// @return Reference to this.
    TVec3& operator*=(T s) noexcept;
    /// Divides every component by scalar s.
    /// @param s Scalar divisor.
    /// @return Reference to this.
    TVec3& operator/=(T s) noexcept;

    /// Returns the squared length.
    /// @return Squared magnitude.
    T squaredLength() const noexcept;
    /// Returns the length.
    /// @return Magnitude.
    T length() const noexcept;

    /// Normalizes this in place.
    /// @return Reference to this.
    TVec3& normalize() noexcept;
    /// Returns a normalized copy.
    /// @return Unit vector in the direction of this.
    TVec3 normalized() const noexcept;

    /// Returns the dot product with o.
    /// @param o Other vector.
    /// @return Dot product.
    constexpr T dot(const TVec3& o) const noexcept { return x * o.x + y * o.y + z * o.z; }
    /// Returns the cross product with o.
    /// @param o Other vector.
    /// @return Cross product.
    constexpr TVec3 cross(const TVec3& o) const noexcept {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }

    /// Returns the distance to o.
    /// @param o Other vector.
    /// @return Euclidean distance.
    T distance(const TVec3& o) const noexcept;
    /// Returns the squared distance to o.
    /// @param o Other vector.
    /// @return Squared distance.
    T squaredDistance(const TVec3& o) const noexcept;

    /// Compares for component-wise equality.
    /// @param o Other vector.
    /// @return True when all components are equal.
    constexpr bool operator==(const TVec3& o) const noexcept {
        return x == o.x && y == o.y && z == o.z;
    }
    /// Compares for component-wise inequality.
    /// @param o Other vector.
    /// @return True when any component differs.
    constexpr bool operator!=(const TVec3& o) const noexcept { return !(*this == o); }
};

/// Adds a and b component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise sum.
template <typename T> constexpr TVec3<T> operator+(const TVec3<T>& a, const TVec3<T>& b) noexcept {
    TVec3<T> r(a);
    r += b;
    return r;
}
/// Subtracts b from a component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise difference.
template <typename T> constexpr TVec3<T> operator-(const TVec3<T>& a, const TVec3<T>& b) noexcept {
    TVec3<T> r(a);
    r -= b;
    return r;
}
/// Multiplies a and b component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise product.
template <typename T> constexpr TVec3<T> operator*(const TVec3<T>& a, const TVec3<T>& b) noexcept {
    TVec3<T> r(a);
    r *= b;
    return r;
}
/// Divides a by b component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise quotient.
template <typename T> constexpr TVec3<T> operator/(const TVec3<T>& a, const TVec3<T>& b) noexcept {
    TVec3<T> r(a);
    r /= b;
    return r;
}
/// Scales a by scalar s.
/// @tparam T component type.
/// @param a Vector to scale.
/// @param s Scalar factor.
/// @return Scaled vector.
template <typename T> constexpr TVec3<T> operator*(const TVec3<T>& a, T s) noexcept {
    TVec3<T> r(a);
    r *= s;
    return r;
}
/// Scales a by scalar s (commutative form).
/// @tparam T component type.
/// @param s Scalar factor.
/// @param a Vector to scale.
/// @return Scaled vector.
template <typename T> constexpr TVec3<T> operator*(T s, const TVec3<T>& a) noexcept {
    return a * s;
}
/// Divides a by scalar s.
/// @tparam T component type.
/// @param a Vector to divide.
/// @param s Scalar divisor.
/// @return Divided vector.
template <typename T> constexpr TVec3<T> operator/(const TVec3<T>& a, T s) noexcept {
    TVec3<T> r(a);
    r /= s;
    return r;
}

/// Four-component vector with component-wise arithmetic.
template <typename T> struct TVec4 {
    /// X component.
    T x = T(0);
    /// Y component.
    T y = T(0);
    /// Z component.
    T z = T(0);
    /// W component.
    T w = T(0);

    /// Default constructor; zero-initializes all components.
    constexpr TVec4() noexcept = default;
    /// Constructs from X, Y, Z and W components.
    /// @param xi X component.
    /// @param yi Y component.
    /// @param zi Z component.
    /// @param wi W component.
    constexpr TVec4(T xi, T yi, T zi, T wi) noexcept : x(xi), y(yi), z(zi), w(wi) {}
    /// Constructs from a TVec3 and an optional W component.
    /// @param v X, Y and Z components.
    /// @param wi W component (defaults to 0).
    constexpr explicit TVec4(const TVec3<T>& v, T wi = T(0)) noexcept
        : x(v.x), y(v.y), z(v.z), w(wi) {}
    /// Constructs a vector with every component set to v.
    /// @param v Value for each component.
    explicit constexpr TVec4(T v) noexcept : x(v), y(v), z(v), w(v) {}

    /// Returns the zero vector.
    /// @return TVec4(0, 0, 0, 0).
    static constexpr TVec4 zero() noexcept { return TVec4(T(0), T(0), T(0), T(0)); }
    /// Returns the one vector.
    /// @return TVec4(1, 1, 1, 1).
    static constexpr TVec4 one() noexcept { return TVec4(T(1), T(1), T(1), T(1)); }

    /// Returns the component at index i (0 = x, 1 = y, 2 = z, 3 = w).
    /// @param i Component index.
    /// @return Component value.
    constexpr T operator[](int i) const noexcept {
        return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
    }
    /// Returns a reference to the component at index i.
    /// @param i Component index.
    /// @return Reference to the component.
    T& operator[](int i) noexcept;

    /// Returns the negated vector.
    /// @return Component-wise negation of this.
    constexpr TVec4 operator-() const noexcept { return {T(-x), T(-y), T(-z), T(-w)}; }

    /// Adds o component-wise.
    /// @param o Vector to add.
    /// @return Reference to this.
    TVec4& operator+=(const TVec4& o) noexcept;
    /// Subtracts o component-wise.
    /// @param o Vector to subtract.
    /// @return Reference to this.
    TVec4& operator-=(const TVec4& o) noexcept;
    /// Multiplies component-wise by o.
    /// @param o Vector to multiply by.
    /// @return Reference to this.
    TVec4& operator*=(const TVec4& o) noexcept;
    /// Divides component-wise by o.
    /// @param o Vector to divide by.
    /// @return Reference to this.
    TVec4& operator/=(const TVec4& o) noexcept;
    /// Multiplies every component by scalar s.
    /// @param s Scalar factor.
    /// @return Reference to this.
    TVec4& operator*=(T s) noexcept;
    /// Divides every component by scalar s.
    /// @param s Scalar divisor.
    /// @return Reference to this.
    TVec4& operator/=(T s) noexcept;

    /// Returns the squared length.
    /// @return Squared magnitude.
    T squaredLength() const noexcept;
    /// Returns the length.
    /// @return Magnitude.
    T length() const noexcept;

    /// Normalizes this in place.
    /// @return Reference to this.
    TVec4& normalize() noexcept;
    /// Returns a normalized copy.
    /// @return Unit vector in the direction of this.
    TVec4 normalized() const noexcept;

    /// Returns the dot product with o.
    /// @param o Other vector.
    /// @return Dot product.
    constexpr T dot(const TVec4& o) const noexcept { return x * o.x + y * o.y + z * o.z + w * o.w; }

    /// Returns the distance to o.
    /// @param o Other vector.
    /// @return Euclidean distance.
    T distance(const TVec4& o) const noexcept;
    /// Returns the squared distance to o.
    /// @param o Other vector.
    /// @return Squared distance.
    T squaredDistance(const TVec4& o) const noexcept;

    /// Compares for component-wise equality.
    /// @param o Other vector.
    /// @return True when all components are equal.
    constexpr bool operator==(const TVec4& o) const noexcept {
        return x == o.x && y == o.y && z == o.z && w == o.w;
    }
    /// Compares for component-wise inequality.
    /// @param o Other vector.
    /// @return True when any component differs.
    constexpr bool operator!=(const TVec4& o) const noexcept { return !(*this == o); }
};

/// Adds a and b component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise sum.
template <typename T> constexpr TVec4<T> operator+(const TVec4<T>& a, const TVec4<T>& b) noexcept {
    TVec4<T> r(a);
    r += b;
    return r;
}
/// Subtracts b from a component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise difference.
template <typename T> constexpr TVec4<T> operator-(const TVec4<T>& a, const TVec4<T>& b) noexcept {
    TVec4<T> r(a);
    r -= b;
    return r;
}
/// Multiplies a and b component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise product.
template <typename T> constexpr TVec4<T> operator*(const TVec4<T>& a, const TVec4<T>& b) noexcept {
    TVec4<T> r(a);
    r *= b;
    return r;
}
/// Divides a by b component-wise.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Component-wise quotient.
template <typename T> constexpr TVec4<T> operator/(const TVec4<T>& a, const TVec4<T>& b) noexcept {
    TVec4<T> r(a);
    r /= b;
    return r;
}
/// Scales a by scalar s.
/// @tparam T component type.
/// @param a Vector to scale.
/// @param s Scalar factor.
/// @return Scaled vector.
template <typename T> constexpr TVec4<T> operator*(const TVec4<T>& a, T s) noexcept {
    TVec4<T> r(a);
    r *= s;
    return r;
}
/// Scales a by scalar s (commutative form).
/// @tparam T component type.
/// @param s Scalar factor.
/// @param a Vector to scale.
/// @return Scaled vector.
template <typename T> constexpr TVec4<T> operator*(T s, const TVec4<T>& a) noexcept {
    TVec4<T> r(a);
    r *= s;
    return r;
}
/// Divides a by scalar s.
/// @tparam T component type.
/// @param a Vector to divide.
/// @param s Scalar divisor.
/// @return Divided vector.
template <typename T> constexpr TVec4<T> operator/(const TVec4<T>& a, T s) noexcept {
    TVec4<T> r(a);
    r /= s;
    return r;
}

/// Float 2D vector.
using Vec2 = TVec2<float>;
/// Float 3D vector.
using Vec3 = TVec3<float>;
/// Float 4D vector.
using Vec4 = TVec4<float>;
/// Double 2D vector.
using DVec2 = TVec2<double>;
/// Double 3D vector.
using DVec3 = TVec3<double>;
/// Double 4D vector.
using DVec4 = TVec4<double>;
/// Signed 32-bit 2D vector.
using IVec2 = TVec2<int32>;
/// Signed 32-bit 3D vector.
using IVec3 = TVec3<int32>;
/// Signed 32-bit 4D vector.
using IVec4 = TVec4<int32>;
/// Unsigned 32-bit 2D vector.
using UVec2 = TVec2<uint32>;
/// Unsigned 32-bit 3D vector.
using UVec3 = TVec3<uint32>;
/// Unsigned 32-bit 4D vector.
using UVec4 = TVec4<uint32>;
/// Unsigned 8-bit 2D vector.
using CVec2 = TVec2<uint8>;
/// Unsigned 8-bit 3D vector.
using CVec3 = TVec3<uint8>;
/// Unsigned 8-bit 4D vector.
using CVec4 = TVec4<uint8>;

namespace detail {
/// Returns true for floating point and integral types.
template <typename T> constexpr bool isScalar() {
    return std::is_floating_point<T>::value || std::is_integral<T>::value;
}
} // namespace detail

/// Component-wise minimum of two 2D vectors.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Vector of per-component minima.
template <typename T>
constexpr TVec2<T> minElementWise(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    return {minVal(a.x, b.x), minVal(a.y, b.y)};
}
/// Component-wise minimum of two 3D vectors.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Vector of per-component minima.
template <typename T>
constexpr TVec3<T> minElementWise(const TVec3<T>& a, const TVec3<T>& b) noexcept {
    return {minVal(a.x, b.x), minVal(a.y, b.y), minVal(a.z, b.z)};
}
/// Component-wise minimum of two 4D vectors.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Vector of per-component minima.
template <typename T>
constexpr TVec4<T> minElementWise(const TVec4<T>& a, const TVec4<T>& b) noexcept {
    return {minVal(a.x, b.x), minVal(a.y, b.y), minVal(a.z, b.z), minVal(a.w, b.w)};
}
/// Component-wise maximum of two 2D vectors.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Vector of per-component maxima.
template <typename T>
constexpr TVec2<T> maxElementWise(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    return {maxVal(a.x, b.x), maxVal(a.y, b.y)};
}
/// Component-wise maximum of two 3D vectors.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Vector of per-component maxima.
template <typename T>
constexpr TVec3<T> maxElementWise(const TVec3<T>& a, const TVec3<T>& b) noexcept {
    return {maxVal(a.x, b.x), maxVal(a.y, b.y), maxVal(a.z, b.z)};
}
/// Component-wise maximum of two 4D vectors.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Vector of per-component maxima.
template <typename T>
constexpr TVec4<T> maxElementWise(const TVec4<T>& a, const TVec4<T>& b) noexcept {
    return {maxVal(a.x, b.x), maxVal(a.y, b.y), maxVal(a.z, b.z), maxVal(a.w, b.w)};
}

/// Clamps each component of a 2D vector to [lo, hi].
/// @tparam T component type.
/// @param v Vector to clamp.
/// @param lo Lower bound per component.
/// @param hi Upper bound per component.
/// @return Clamped vector.
template <typename T>
constexpr TVec2<T>
clampComponentwise(const TVec2<T>& v, const TVec2<T>& lo, const TVec2<T>& hi) noexcept {
    return {clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y)};
}
/// Clamps each component of a 3D vector to [lo, hi].
/// @tparam T component type.
/// @param v Vector to clamp.
/// @param lo Lower bound per component.
/// @param hi Upper bound per component.
/// @return Clamped vector.
template <typename T>
constexpr TVec3<T>
clampComponentwise(const TVec3<T>& v, const TVec3<T>& lo, const TVec3<T>& hi) noexcept {
    return {clamp(v.x, lo.x, hi.x), clamp(v.y, lo.y, hi.y), clamp(v.z, lo.z, hi.z)};
}
/// Clamps each component of a 4D vector to [lo, hi].
/// @tparam T component type.
/// @param v Vector to clamp.
/// @param lo Lower bound per component.
/// @param hi Upper bound per component.
/// @return Clamped vector.
template <typename T>
constexpr TVec4<T>
clampComponentwise(const TVec4<T>& v, const TVec4<T>& lo, const TVec4<T>& hi) noexcept {
    return {
        clamp(v.x, lo.x, hi.x),
        clamp(v.y, lo.y, hi.y),
        clamp(v.z, lo.z, hi.z),
        clamp(v.w, lo.w, hi.w)};
}

/// Element-wise abs.
/// @tparam T component type.
/// @param v Input vector.
/// @return Vector of per-component absolute values.
template <typename T> constexpr TVec2<T> absVec(const TVec2<T>& v) noexcept {
    return {absVal(v.x), absVal(v.y)};
}
/// Element-wise abs.
/// @tparam T component type.
/// @param v Input vector.
/// @return Vector of per-component absolute values.
template <typename T> constexpr TVec3<T> absVec(const TVec3<T>& v) noexcept {
    return {absVal(v.x), absVal(v.y), absVal(v.z)};
}
/// Element-wise abs.
/// @tparam T component type.
/// @param v Input vector.
/// @return Vector of per-component absolute values.
template <typename T> constexpr TVec4<T> absVec(const TVec4<T>& v) noexcept {
    return {absVal(v.x), absVal(v.y), absVal(v.z), absVal(v.w)};
}

/// Dot product of two 2D vectors.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Dot product.
template <typename T> constexpr T dot(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    return a.dot(b);
}
/// Dot product of two 3D vectors.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Dot product.
template <typename T> constexpr T dot(const TVec3<T>& a, const TVec3<T>& b) noexcept {
    return a.dot(b);
}
/// Dot product of two 4D vectors.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Dot product.
template <typename T> constexpr T dot(const TVec4<T>& a, const TVec4<T>& b) noexcept {
    return a.dot(b);
}

/// Cross product of two 3D vectors.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Cross product.
template <typename T> constexpr TVec3<T> cross(const TVec3<T>& a, const TVec3<T>& b) noexcept {
    return a.cross(b);
}

/// 2D cross returns the scalar z of the cross product of the 3D extensions.
/// @tparam T component type.
/// @param a First vector.
/// @param b Second vector.
/// @return Signed area scalar (a.x * b.y - a.y * b.x).
template <typename T> constexpr T cross(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    return a.x * b.y - a.y * b.x;
}

/// Length of a 2D vector.
/// @tparam T component type.
/// @param v Vector.
/// @return Magnitude.
template <typename T> T length(const TVec2<T>& v) noexcept;
/// Length of a 3D vector.
/// @tparam T component type.
/// @param v Vector.
/// @return Magnitude.
template <typename T> T length(const TVec3<T>& v) noexcept;
/// Length of a 4D vector.
/// @tparam T component type.
/// @param v Vector.
/// @return Magnitude.
template <typename T> T length(const TVec4<T>& v) noexcept;

/// Normalized copy of a 2D vector.
/// @tparam T component type.
/// @param v Vector to normalize.
/// @return Unit vector.
template <typename T> TVec2<T> normalize(const TVec2<T>& v) noexcept;
/// Normalized copy of a 3D vector.
/// @tparam T component type.
/// @param v Vector to normalize.
/// @return Unit vector.
template <typename T> TVec3<T> normalize(const TVec3<T>& v) noexcept;
/// Normalized copy of a 4D vector.
/// @tparam T component type.
/// @param v Vector to normalize.
/// @return Unit vector.
template <typename T> TVec4<T> normalize(const TVec4<T>& v) noexcept;

/// Distance between two 2D points.
/// @tparam T component type.
/// @param a First point.
/// @param b Second point.
/// @return Euclidean distance.
template <typename T> T distance(const TVec2<T>& a, const TVec2<T>& b) noexcept;
/// Distance between two 3D points.
/// @tparam T component type.
/// @param a First point.
/// @param b Second point.
/// @return Euclidean distance.
template <typename T> T distance(const TVec3<T>& a, const TVec3<T>& b) noexcept;

/// Reflects v about the unit normal n: v - 2*(v.n)*n.
/// @tparam T component type.
/// @param v Vector to reflect.
/// @param n Unit normal.
/// @return Reflected vector.
template <typename T> TVec2<T> reflect(const TVec2<T>& v, const TVec2<T>& n) noexcept;
/// Reflects v about the unit normal n: v - 2*(v.n)*n.
/// @tparam T component type.
/// @param v Vector to reflect.
/// @param n Unit normal.
/// @return Reflected vector.
template <typename T> TVec3<T> reflect(const TVec3<T>& v, const TVec3<T>& n) noexcept;

/// Projects v onto the direction d (d need not be normalized).
/// @tparam T component type.
/// @param v Vector to project.
/// @param d Projection direction.
/// @return Projection of v onto d.
template <typename T> TVec2<T> project(const TVec2<T>& v, const TVec2<T>& d) noexcept;
/// Projects v onto the direction d (d need not be normalized).
/// @tparam T component type.
/// @param v Vector to project.
/// @param d Projection direction.
/// @return Projection of v onto d.
template <typename T> TVec3<T> project(const TVec3<T>& v, const TVec3<T>& d) noexcept;

} // namespace math
} // namespace iml

#include "math/Vector.tpp"
