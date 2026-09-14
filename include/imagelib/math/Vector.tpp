#ifndef IMAGELIB_MATH_VECTOR_H_
#error "Include imagelib/math/Vector.h, not this .tpp directly."
#endif

namespace iml {
namespace math {

// ---------------------------------------------------------------------------
// TVec2
// ---------------------------------------------------------------------------

template <typename T>
T& TVec2<T>::operator[](int i) noexcept { return i == 0 ? x : y; }

template <typename T>
TVec2<T>& TVec2<T>::operator+=(const TVec2<T>& o) noexcept { x += o.x; y += o.y; return *this; }
template <typename T>
TVec2<T>& TVec2<T>::operator-=(const TVec2<T>& o) noexcept { x -= o.x; y -= o.y; return *this; }
template <typename T>
TVec2<T>& TVec2<T>::operator*=(const TVec2<T>& o) noexcept { x *= o.x; y *= o.y; return *this; }
template <typename T>
TVec2<T>& TVec2<T>::operator/=(const TVec2<T>& o) noexcept { x /= o.x; y /= o.y; return *this; }
template <typename T>
TVec2<T>& TVec2<T>::operator*=(T s) noexcept { x *= s; y *= s; return *this; }
template <typename T>
TVec2<T>& TVec2<T>::operator/=(T s) noexcept { x /= s; y /= s; return *this; }

template <typename T>
T TVec2<T>::squaredLength() const noexcept { return x * x + y * y; }
template <typename T>
T TVec2<T>::length() const noexcept { return std::sqrt(squaredLength()); }

template <typename T>
TVec2<T>& TVec2<T>::normalize() noexcept {
    T len = length();
    if (len > T(0)) { x /= len; y /= len; }
    return *this;
}
template <typename T>
TVec2<T> TVec2<T>::normalized() const noexcept {
    TVec2<T> r(*this); return r.normalize();
}

template <typename T>
T TVec2<T>::distance(const TVec2<T>& o) const noexcept { return (*this - o).length(); }
template <typename T>
T TVec2<T>::squaredDistance(const TVec2<T>& o) const noexcept { return (*this - o).squaredLength(); }

// ---------------------------------------------------------------------------
// TVec3
// ---------------------------------------------------------------------------

template <typename T>
T& TVec3<T>::operator[](int i) noexcept { return i == 0 ? x : (i == 1 ? y : z); }

template <typename T>
TVec3<T>& TVec3<T>::operator+=(const TVec3<T>& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }
template <typename T>
TVec3<T>& TVec3<T>::operator-=(const TVec3<T>& o) noexcept { x -= o.x; y -= o.y; z -= o.z; return *this; }
template <typename T>
TVec3<T>& TVec3<T>::operator*=(const TVec3<T>& o) noexcept { x *= o.x; y *= o.y; z *= o.z; return *this; }
template <typename T>
TVec3<T>& TVec3<T>::operator/=(const TVec3<T>& o) noexcept { x /= o.x; y /= o.y; z /= o.z; return *this; }
template <typename T>
TVec3<T>& TVec3<T>::operator*=(T s) noexcept { x *= s; y *= s; z *= s; return *this; }
template <typename T>
TVec3<T>& TVec3<T>::operator/=(T s) noexcept { x /= s; y /= s; z /= s; return *this; }

template <typename T>
T TVec3<T>::squaredLength() const noexcept { return x * x + y * y + z * z; }
template <typename T>
T TVec3<T>::length() const noexcept { return std::sqrt(squaredLength()); }

template <typename T>
TVec3<T>& TVec3<T>::normalize() noexcept {
    T len = length();
    if (len > T(0)) { x /= len; y /= len; z /= len; }
    return *this;
}
template <typename T>
TVec3<T> TVec3<T>::normalized() const noexcept { TVec3<T> r(*this); return r.normalize(); }

template <typename T>
T TVec3<T>::distance(const TVec3<T>& o) const noexcept { return (*this - o).length(); }
template <typename T>
T TVec3<T>::squaredDistance(const TVec3<T>& o) const noexcept { return (*this - o).squaredLength(); }

// ---------------------------------------------------------------------------
// TVec4
// ---------------------------------------------------------------------------

template <typename T>
T& TVec4<T>::operator[](int i) noexcept { return i == 0 ? x : (i == 1 ? y : (i == 2 ? z : w)); }

template <typename T>
TVec4<T>& TVec4<T>::operator+=(const TVec4<T>& o) noexcept { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
template <typename T>
TVec4<T>& TVec4<T>::operator-=(const TVec4<T>& o) noexcept { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
template <typename T>
TVec4<T>& TVec4<T>::operator*=(const TVec4<T>& o) noexcept { x *= o.x; y *= o.y; z *= o.z; w *= o.w; return *this; }
template <typename T>
TVec4<T>& TVec4<T>::operator/=(const TVec4<T>& o) noexcept { x /= o.x; y /= o.y; z /= o.z; w /= o.w; return *this; }
template <typename T>
TVec4<T>& TVec4<T>::operator*=(T s) noexcept { x *= s; y *= s; z *= s; w *= s; return *this; }
template <typename T>
TVec4<T>& TVec4<T>::operator/=(T s) noexcept { x /= s; y /= s; z /= s; w /= s; return *this; }

template <typename T>
T TVec4<T>::squaredLength() const noexcept { return x * x + y * y + z * z + w * w; }
template <typename T>
T TVec4<T>::length() const noexcept { return std::sqrt(squaredLength()); }

template <typename T>
TVec4<T>& TVec4<T>::normalize() noexcept {
    T len = length();
    if (len > T(0)) { x /= len; y /= len; z /= len; w /= len; }
    return *this;
}
template <typename T>
TVec4<T> TVec4<T>::normalized() const noexcept { TVec4<T> r(*this); return r.normalize(); }

template <typename T>
T TVec4<T>::distance(const TVec4<T>& o) const noexcept { return (*this - o).length(); }
template <typename T>
T TVec4<T>::squaredDistance(const TVec4<T>& o) const noexcept { return (*this - o).squaredLength(); }

// ---------------------------------------------------------------------------
// Free-function vector math
// ---------------------------------------------------------------------------

template <typename T>
T length(const TVec2<T>& v) noexcept { return v.length(); }
template <typename T>
T length(const TVec3<T>& v) noexcept { return v.length(); }
template <typename T>
T length(const TVec4<T>& v) noexcept { return v.length(); }

template <typename T>
TVec2<T> normalize(const TVec2<T>& v) noexcept { return v.normalized(); }
template <typename T>
TVec3<T> normalize(const TVec3<T>& v) noexcept { return v.normalized(); }
template <typename T>
TVec4<T> normalize(const TVec4<T>& v) noexcept { return v.normalized(); }

template <typename T>
T distance(const TVec2<T>& a, const TVec2<T>& b) noexcept { return a.distance(b); }
template <typename T>
T distance(const TVec3<T>& a, const TVec3<T>& b) noexcept { return a.distance(b); }

/// Reflects v about the unit normal n: v - 2*(v.n)*n.
template <typename T>
TVec2<T> reflect(const TVec2<T>& v, const TVec2<T>& n) noexcept {
    return v - n * (T(2) * v.dot(n));
}
template <typename T>
TVec3<T> reflect(const TVec3<T>& v, const TVec3<T>& n) noexcept {
    return v - n * (T(2) * v.dot(n));
}

/// Projects v onto the direction d (d need not be normalized).
template <typename T>
TVec2<T> project(const TVec2<T>& v, const TVec2<T>& d) noexcept {
    T s2 = d.squaredLength();
    if (s2 <= T(0)) return TVec2<T>::zero();
    return d * (v.dot(d) / s2);
}
template <typename T>
TVec3<T> project(const TVec3<T>& v, const TVec3<T>& d) noexcept {
    T s2 = d.squaredLength();
    if (s2 <= T(0)) return TVec3<T>::zero();
    return d * (v.dot(d) / s2);
}

} // namespace math
} // namespace iml