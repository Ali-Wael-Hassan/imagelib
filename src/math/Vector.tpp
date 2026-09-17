#ifndef IMAGELIB_MATH_VECTOR_H_
#error "Include imagelib/math/Vector.h, not this .tpp directly."
#endif

namespace iml {
namespace math {

/// Returns a reference to the component at index i (0 = x, 1 = y).
/// @param i Component index.
/// @return Reference to x or y.
template <typename T> T& TVec2<T>::operator[](int i) noexcept { return i == 0 ? x : y; }

/// Adds o component-wise.
/// @param o Vector to add.
/// @return This vector.
template <typename T> TVec2<T>& TVec2<T>::operator+=(const TVec2<T>& o) noexcept {
    x += o.x;
    y += o.y;
    return *this;
}
/// Subtracts o component-wise.
/// @param o Vector to subtract.
/// @return This vector.
template <typename T> TVec2<T>& TVec2<T>::operator-=(const TVec2<T>& o) noexcept {
    x -= o.x;
    y -= o.y;
    return *this;
}
/// Multiplies component-wise by o.
/// @param o Vector to multiply by.
/// @return This vector.
template <typename T> TVec2<T>& TVec2<T>::operator*=(const TVec2<T>& o) noexcept {
    x *= o.x;
    y *= o.y;
    return *this;
}
/// Divides component-wise by o.
/// @param o Vector to divide by.
/// @return This vector.
template <typename T> TVec2<T>& TVec2<T>::operator/=(const TVec2<T>& o) noexcept {
    x /= o.x;
    y /= o.y;
    return *this;
}
/// Scales all components by s.
/// @param s Scale factor.
/// @return This vector.
template <typename T> TVec2<T>& TVec2<T>::operator*=(T s) noexcept {
    x *= s;
    y *= s;
    return *this;
}
/// Divides all components by s.
/// @param s Divisor.
/// @return This vector.
template <typename T> TVec2<T>& TVec2<T>::operator/=(T s) noexcept {
    x /= s;
    y /= s;
    return *this;
}

/// Returns the squared length without a square root.
/// @return The squared length.
template <typename T> T TVec2<T>::squaredLength() const noexcept { return x * x + y * y; }
/// Returns the Euclidean length.
/// @return The length.
template <typename T> T TVec2<T>::length() const noexcept { return std::sqrt(squaredLength()); }

/// Normalizes in place; leaves a zero vector unchanged.
/// @return This vector.
template <typename T> TVec2<T>& TVec2<T>::normalize() noexcept {
    T len = length();
    if (len > T(0)) {
        x /= len;
        y /= len;
    }
    return *this;
}
/// Returns a normalized copy.
/// @return The normalized vector.
template <typename T> TVec2<T> TVec2<T>::normalized() const noexcept {
    TVec2<T> r(*this);
    return r.normalize();
}

/// Returns the Euclidean distance to o.
/// @param o Other vector.
/// @return The distance.
template <typename T> T TVec2<T>::distance(const TVec2<T>& o) const noexcept {
    return (*this - o).length();
}
/// Returns the squared distance to o.
/// @param o Other vector.
/// @return The squared distance.
template <typename T> T TVec2<T>::squaredDistance(const TVec2<T>& o) const noexcept {
    return (*this - o).squaredLength();
}

/// Returns a reference to the component at index i (0 = x, 1 = y, 2 = z).
/// @param i Component index.
/// @return Reference to x, y, or z.
template <typename T> T& TVec3<T>::operator[](int i) noexcept {
    return i == 0 ? x : (i == 1 ? y : z);
}

/// Adds o component-wise.
/// @param o Vector to add.
/// @return This vector.
template <typename T> TVec3<T>& TVec3<T>::operator+=(const TVec3<T>& o) noexcept {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
}
/// Subtracts o component-wise.
/// @param o Vector to subtract.
/// @return This vector.
template <typename T> TVec3<T>& TVec3<T>::operator-=(const TVec3<T>& o) noexcept {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
}
/// Multiplies component-wise by o.
/// @param o Vector to multiply by.
/// @return This vector.
template <typename T> TVec3<T>& TVec3<T>::operator*=(const TVec3<T>& o) noexcept {
    x *= o.x;
    y *= o.y;
    z *= o.z;
    return *this;
}
/// Divides component-wise by o.
/// @param o Vector to divide by.
/// @return This vector.
template <typename T> TVec3<T>& TVec3<T>::operator/=(const TVec3<T>& o) noexcept {
    x /= o.x;
    y /= o.y;
    z /= o.z;
    return *this;
}
/// Scales all components by s.
/// @param s Scale factor.
/// @return This vector.
template <typename T> TVec3<T>& TVec3<T>::operator*=(T s) noexcept {
    x *= s;
    y *= s;
    z *= s;
    return *this;
}
/// Divides all components by s.
/// @param s Divisor.
/// @return This vector.
template <typename T> TVec3<T>& TVec3<T>::operator/=(T s) noexcept {
    x /= s;
    y /= s;
    z /= s;
    return *this;
}

/// Returns the squared length without a square root.
/// @return The squared length.
template <typename T> T TVec3<T>::squaredLength() const noexcept { return x * x + y * y + z * z; }
/// Returns the Euclidean length.
/// @return The length.
template <typename T> T TVec3<T>::length() const noexcept { return std::sqrt(squaredLength()); }

/// Normalizes in place; leaves a zero vector unchanged.
/// @return This vector.
template <typename T> TVec3<T>& TVec3<T>::normalize() noexcept {
    T len = length();
    if (len > T(0)) {
        x /= len;
        y /= len;
        z /= len;
    }
    return *this;
}
/// Returns a normalized copy.
/// @return The normalized vector.
template <typename T> TVec3<T> TVec3<T>::normalized() const noexcept {
    TVec3<T> r(*this);
    return r.normalize();
}

/// Returns the Euclidean distance to o.
/// @param o Other vector.
/// @return The distance.
template <typename T> T TVec3<T>::distance(const TVec3<T>& o) const noexcept {
    return (*this - o).length();
}
/// Returns the squared distance to o.
/// @param o Other vector.
/// @return The squared distance.
template <typename T> T TVec3<T>::squaredDistance(const TVec3<T>& o) const noexcept {
    return (*this - o).squaredLength();
}

/// Returns a reference to the component at index i (0 = x, 1 = y, 2 = z, 3 = w).
/// @param i Component index.
/// @return Reference to x, y, z, or w.
template <typename T> T& TVec4<T>::operator[](int i) noexcept {
    return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w));
}

/// Adds o component-wise.
/// @param o Vector to add.
/// @return This vector.
template <typename T> TVec4<T>& TVec4<T>::operator+=(const TVec4<T>& o) noexcept {
    x += o.x;
    y += o.y;
    z += o.z;
    w += o.w;
    return *this;
}
/// Subtracts o component-wise.
/// @param o Vector to subtract.
/// @return This vector.
template <typename T> TVec4<T>& TVec4<T>::operator-=(const TVec4<T>& o) noexcept {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    w -= o.w;
    return *this;
}
/// Multiplies component-wise by o.
/// @param o Vector to multiply by.
/// @return This vector.
template <typename T> TVec4<T>& TVec4<T>::operator*=(const TVec4<T>& o) noexcept {
    x *= o.x;
    y *= o.y;
    z *= o.z;
    w *= o.w;
    return *this;
}
/// Divides component-wise by o.
/// @param o Vector to divide by.
/// @return This vector.
template <typename T> TVec4<T>& TVec4<T>::operator/=(const TVec4<T>& o) noexcept {
    x /= o.x;
    y /= o.y;
    z /= o.z;
    w /= o.w;
    return *this;
}
/// Scales all components by s.
/// @param s Scale factor.
/// @return This vector.
template <typename T> TVec4<T>& TVec4<T>::operator*=(T s) noexcept {
    x *= s;
    y *= s;
    z *= s;
    w *= s;
    return *this;
}
/// Divides all components by s.
/// @param s Divisor.
/// @return This vector.
template <typename T> TVec4<T>& TVec4<T>::operator/=(T s) noexcept {
    x /= s;
    y /= s;
    z /= s;
    w /= s;
    return *this;
}

/// Returns the squared length without a square root.
/// @return The squared length.
template <typename T> T TVec4<T>::squaredLength() const noexcept {
    return x * x + y * y + z * z + w * w;
}
/// Returns the Euclidean length.
/// @return The length.
template <typename T> T TVec4<T>::length() const noexcept { return std::sqrt(squaredLength()); }

/// Normalizes in place; leaves a zero vector unchanged.
/// @return This vector.
template <typename T> TVec4<T>& TVec4<T>::normalize() noexcept {
    T len = length();
    if (len > T(0)) {
        x /= len;
        y /= len;
        z /= len;
        w /= len;
    }
    return *this;
}
/// Returns a normalized copy.
/// @return The normalized vector.
template <typename T> TVec4<T> TVec4<T>::normalized() const noexcept {
    TVec4<T> r(*this);
    return r.normalize();
}

/// Returns the Euclidean distance to o.
/// @param o Other vector.
/// @return The distance.
template <typename T> T TVec4<T>::distance(const TVec4<T>& o) const noexcept {
    return (*this - o).length();
}
/// Returns the squared distance to o.
/// @param o Other vector.
/// @return The squared distance.
template <typename T> T TVec4<T>::squaredDistance(const TVec4<T>& o) const noexcept {
    return (*this - o).squaredLength();
}

/// Returns the length of v.
/// @tparam T Scalar component type.
/// @param v Input vector.
/// @return The Euclidean length.
template <typename T> T length(const TVec2<T>& v) noexcept { return v.length(); }
/// Returns the length of v.
/// @tparam T Scalar component type.
/// @param v Input vector.
/// @return The Euclidean length.
template <typename T> T length(const TVec3<T>& v) noexcept { return v.length(); }
/// Returns the length of v.
/// @tparam T Scalar component type.
/// @param v Input vector.
/// @return The Euclidean length.
template <typename T> T length(const TVec4<T>& v) noexcept { return v.length(); }

/// Returns a normalized copy of v.
/// @tparam T Scalar component type.
/// @param v Input vector.
/// @return The normalized vector.
template <typename T> TVec2<T> normalize(const TVec2<T>& v) noexcept { return v.normalized(); }
/// Returns a normalized copy of v.
/// @tparam T Scalar component type.
/// @param v Input vector.
/// @return The normalized vector.
template <typename T> TVec3<T> normalize(const TVec3<T>& v) noexcept { return v.normalized(); }
/// Returns a normalized copy of v.
/// @tparam T Scalar component type.
/// @param v Input vector.
/// @return The normalized vector.
template <typename T> TVec4<T> normalize(const TVec4<T>& v) noexcept { return v.normalized(); }

/// Returns the distance between a and b.
/// @tparam T Scalar component type.
/// @param a First point.
/// @param b Second point.
/// @return The distance.
template <typename T> T distance(const TVec2<T>& a, const TVec2<T>& b) noexcept {
    return a.distance(b);
}
/// Returns the distance between a and b.
/// @tparam T Scalar component type.
/// @param a First point.
/// @param b Second point.
/// @return The distance.
template <typename T> T distance(const TVec3<T>& a, const TVec3<T>& b) noexcept {
    return a.distance(b);
}

/// Reflects v about the unit normal n: v - 2*(v.n)*n.
template <typename T> TVec2<T> reflect(const TVec2<T>& v, const TVec2<T>& n) noexcept {
    return v - n * (T(2) * v.dot(n));
}
template <typename T> TVec3<T> reflect(const TVec3<T>& v, const TVec3<T>& n) noexcept {
    return v - n * (T(2) * v.dot(n));
}

/// Projects v onto the direction d (d need not be normalized).
template <typename T> TVec2<T> project(const TVec2<T>& v, const TVec2<T>& d) noexcept {
    T s2 = d.squaredLength();
    if (s2 <= T(0))
        return TVec2<T>::zero();
    return d * (v.dot(d) / s2);
}
template <typename T> TVec3<T> project(const TVec3<T>& v, const TVec3<T>& d) noexcept {
    T s2 = d.squaredLength();
    if (s2 <= T(0))
        return TVec3<T>::zero();
    return d * (v.dot(d) / s2);
}

} // namespace math
} // namespace iml