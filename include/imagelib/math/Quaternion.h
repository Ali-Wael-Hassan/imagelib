#pragma once
// imagelib/math/Quaternion.h
//
// Quaternion math for rotations. Head-only math value type. The Quat<->Mat4
// bridges live in src/math/Matrix.cpp (avoids an include cycle).

#include "imagelib/core/Types.h"
#include "imagelib/math/Scalar.h"
#include "imagelib/math/Vector.h"

#include <cmath>

namespace iml {
namespace math {

struct Mat4; // fwd

struct Quat {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    constexpr Quat() noexcept = default;
    constexpr Quat(float xi, float yi, float zi, float wi) noexcept
        : x(xi), y(yi), z(zi), w(wi) {}
    explicit Quat(const TVec3<float>& axis, float radians) noexcept { *this = fromAxisAngle(axis, radians); }

    static Quat identity() noexcept { return Quat(0, 0, 0, 1); }
    static Quat fromAxisAngle(const TVec3<float>& axis, float radians) noexcept {
        TVec3<float> a = axis.normalized();
        float half = radians * 0.5f;
        float s = std::sin(half), c = std::cos(half);
        return Quat(a.x * s, a.y * s, a.z * s, c);
    }
    /// XYZ intrinsic Euler rotation (radians), applied x then y then z.
    static Quat fromEulerXYZ(float rx, float ry, float rz) noexcept {
        float hx = rx * 0.5f, hy = ry * 0.5f, hz = rz * 0.5f;
        float cx = std::cos(hx), sx = std::sin(hx);
        float cy = std::cos(hy), sy = std::sin(hy);
        float cz = std::cos(hz), sz = std::sin(hz);
        return Quat(
            sx*cy*cz - cx*sy*sz,
            cx*sy*cz + sx*cy*sz,
            cx*cy*sz - sx*sy*cz,
            cx*cy*cz + sx*sy*sz);
    }

    constexpr Quat operator-() const noexcept { return Quat(-x, -y, -z, -w); }

    Quat operator*(const Quat& o) const noexcept {
        return Quat(
            w*o.x + x*o.w + y*o.z - z*o.y,
            w*o.y - x*o.z + y*o.w + z*o.x,
            w*o.z + x*o.y - y*o.x + z*o.w,
            w*o.w - x*o.x - y*o.y - z*o.z);
    }
    Quat& operator*=(const Quat& o) noexcept { *this = (*this) * o; return *this; }

    float dot(const Quat& o) const noexcept { return x*o.x + y*o.y + z*o.z + w*o.w; }
    float squaredLength() const noexcept { return dot(*this); }
    float length() const noexcept { return std::sqrt(squaredLength()); }

    constexpr Quat conjugated() const noexcept { return Quat(-x, -y, -z, w); }
    Quat normalized() const noexcept {
        float len = length();
        constexpr float eps = 1e-8f;
        if (len < eps) return identity();
        return Quat(x/len, y/len, z/len, w/len);
    }
    Quat inverse() const noexcept {
        float s2 = squaredLength();
        if (s2 < 1e-8f) return identity();
        Quat c = conjugated();
        return Quat(c.x/s2, c.y/s2, c.z/s2, c.w/s2);
    }

    /// Rotates a vector by this (unit) quaternion, no matrix.
    TVec3<float> rotate(const TVec3<float>& v) const noexcept {
        Quat q = normalized();
        TVec3<float> qv(q.x, q.y, q.z);
        TVec3<float> t = qv.cross(v) * 2.0f;
        return v + q.w * t + qv.cross(t);
    }

    /// Yaw (about Z), pitch (about Y), roll (about X), radians.
    TVec3<float> toEuler() const noexcept {
        const Quat q = normalized();
        float wp = q.w, xp = q.x, yp = q.y, zp = q.z;
        float roll  = std::atan2(2*(wp*xp + yp*zp), 1 - 2*(xp*xp + yp*yp));
        float sinp  = 2*(wp*yp - zp*xp);
        float pitch = (std::abs(sinp) >= 1) ? std::copysign(halfPiF, sinp)
                                            : std::asin(sinp);
        float yaw   = std::atan2(2*(wp*zp + xp*yp), 1 - 2*(yp*yp + zp*zp));
        return TVec3<float>(roll, pitch, yaw);
    }

    /// Converts to a 4x4 rotation matrix (out-of-line, Matrix.cpp).
    Mat4 toMat4() const noexcept;

    static float lerpAngle(float a, float b, float t) noexcept {
        float d = mod(b - a, twoPiF);
        if (d > piF) d -= twoPiF;
        return a + d * t;
    }
    /// Spherical interpolation; handles the shortest path and near-identical
    /// endpoints.
    static Quat slerp(const Quat& a, const Quat& b, float t) noexcept {
        float d = a.dot(b);
        Quat  bb = b;
        if (d < 0) { d = -d; bb = -b; }
        constexpr float eps = 1e-6f;
        if (d > 1 - eps) {
            return Quat::nlerp(a, bb, t); // nearly parallel
        }
        float theta  = std::acos(clamp(d, -1.0f, 1.0f));
        float sinTh  = std::sin(theta);
        float k0 = std::sin((1 - t) * theta) / sinTh;
        float k1 = std::sin(t * theta) / sinTh;
        Quat r(k0*a.x + k1*bb.x, k0*a.y + k1*bb.y, k0*a.z + k1*bb.z, k0*a.w + k1*bb.w);
        return r.normalized();
    }
    static Quat nlerp(const Quat& a, const Quat& b, float t) noexcept {
        float d = a.dot(b);
        Quat  bb = b;
        if (d < 0) { bb = -b; }
        Quat r(lerp(a.x, bb.x, t), lerp(a.y, bb.y, t),
               lerp(a.z, bb.z, t), lerp(a.w, bb.w, t));
        return r.normalized();
    }
};

} // namespace math
} // namespace iml