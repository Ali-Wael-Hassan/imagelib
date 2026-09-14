// imagelib/src/math/Matrix.cpp
//
// Out-of-line Mat4 math (inverse via cofactors) and the Quat<->Mat4 bridges.
// Everything else in Matrix.h is header-only.

#include "imagelib/math/Matrix.h"
#include "imagelib/math/Quaternion.h"

#include <cmath>
#include <cstring>

namespace iml {
namespace math {

namespace {

// Determinant of the 3x3 sub-matrix formed by deleting row `r` and col `c`.
float minor3(const float* m, int r, int c) {
    int colIdx[3], rowIdx[3], n = 0, m2 = 0;
    for (int i = 0; i < 4; ++i) {
        if (i != c) colIdx[n++] = i;
        if (i != r) rowIdx[m2++] = i;
    }
    auto sub = [&](int i, int j) { return m[colIdx[j] * 4 + rowIdx[i]]; };
    const float& a = sub(0,0); const float& b = sub(0,1); const float& d = sub(0,2);
    const float& e = sub(1,0); const float& f = sub(1,1); const float& g = sub(1,2);
    const float& h = sub(2,0); const float& i_ = sub(2,1); const float& j = sub(2,2);
    return a * (f*j - g*i_) - b * (e*j - g*h) + d * (e*i_ - f*h);
}

float cofactor(const float* m, int r, int c) {
    float det = minor3(m, r, c);
    return ((r + c) & 1) ? -det : det;
}

} // namespace

float Mat4::determinant() const noexcept {
    return m[0]*cofactor(m, 0, 0)
         + m[4]*cofactor(m, 0, 1)
         + m[8]*cofactor(m, 0, 2)
         + m[12]*cofactor(m, 0, 3);
}

Mat4 Mat4::adjugate() const noexcept {
    // adjugate[col][row] == C_{row,col}: cofactor transposed.
    Mat4 r;
    for (int c = 0; c < 4; ++c)
        for (int rw = 0; rw < 4; ++rw)
            r(c, rw) = cofactor(m, c, rw);
    return r;
}

Mat4 Mat4::inverse() const noexcept {
    float det = determinant();
    constexpr float eps = 1e-12f;
    Mat4 adj = adjugate();
    if (std::abs(det) < eps) {
        // Degenerate: fall back to a "best effort" scaling so algorithms
        // (e.g. affine remap) don't explode.
        det = (det < 0) ? -eps : eps;
    }
    for (int i = 0; i < 16; ++i) adj.m[i] /= det;
    return adj;
}

Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) noexcept {
    Vec3 fwd = (center - eye).normalized();
    Vec3 right = fwd.cross(up).normalized();
    if (right.length() < 1e-6f) { // degenerate: up parallel to view
        right = Vec3(1, 0, 0);
    }
    Vec3 up2 = right.cross(fwd);

    Mat4 r;
    r(0,0)=right.x;   r(0,1)=up2.x;   r(0,2)=fwd.x;   r(0,3)=0;
    r(1,0)=right.y;   r(1,1)=up2.y;   r(1,2)=fwd.y;   r(1,3)=0;
    r(2,0)=right.z;   r(2,1)=up2.z;   r(2,2)=fwd.z;   r(2,3)=0;
    r(3,0)=-right.dot(eye);
    r(3,1)=-up2.dot(eye);
    r(3,2)=-fwd.dot(eye);
    r(3,3)=1;
    return r;
}

// ---------------------------------------------------------------------------
// Quat <-> Mat4 bridges
// ---------------------------------------------------------------------------

Mat4 Mat4::fromQuaternion(const Quat& q) noexcept {
    Mat4 r; // identity default
    float x = q.x, y = q.y, z = q.z, w = q.w;

    r(0,0) = 1 - 2*(y*y + z*z); r(0,1) = 2*(x*y + z*w);     r(0,2) = 2*(x*z - y*w);
    r(1,0) = 2*(x*y - z*w);     r(1,1) = 1 - 2*(x*x + z*z); r(1,2) = 2*(y*z + x*w);
    r(2,0) = 2*(x*z + y*w);     r(2,1) = 2*(y*z - x*w);     r(2,2) = 1 - 2*(x*x + y*y);

    r(0,3) = 0; r(1,3) = 0; r(2,3) = 0;
    r(3,0) = 0; r(3,1) = 0; r(3,2) = 0; r(3,3) = 1;
    return r;
}

Mat4 Mat4::compose(const Vec3& t, const Quat& r, const Vec3& s) noexcept {
    return Mat4::translation(t) * Mat4::fromQuaternion(r) * Mat4::scale(s);
}

Mat4 Quat::toMat4() const noexcept {
    return Mat4::fromQuaternion(*this);
}

// ---------------------------------------------------------------------------
// Mat2
// ---------------------------------------------------------------------------

Mat2::Mat2() noexcept { setIdentity(); }
Mat2::Mat2(float s) noexcept : m{ s, 0, 0, s } {}

Mat2 Mat2::zero() noexcept { Mat2 r{}; std::memset(r.m, 0, sizeof(r.m)); return r; }
Mat2 Mat2::identity() noexcept { return Mat2(); }

void Mat2::setIdentity() noexcept { std::memset(m, 0, sizeof(m)); m[0]=1; m[3]=1; }

float& Mat2::operator()(int col, int row) noexcept { return m[col * 2 + row]; }
const float& Mat2::operator()(int col, int row) const noexcept { return m[col * 2 + row]; }

Mat2 Mat2::operator*(const Mat2& b) const noexcept {
    Mat2 r = Mat2::zero();
    for (int c = 0; c < 2; ++c)
        for (int rw = 0; rw < 2; ++rw) {
            float sum = 0;
            for (int k = 0; k < 2; ++k) sum += (*this)(k, rw) * b(c, k);
            r(c, rw) = sum;
        }
    return r;
}
TVec2<float> Mat2::operator*(const TVec2<float>& v) const noexcept {
    return { m[0]*v.x + m[2]*v.y, m[1]*v.x + m[3]*v.y };
}
Mat2 Mat2::transpose() const noexcept {
    Mat2 r;
    for (int c = 0; c < 2; ++c) for (int rw = 0; rw < 2; ++rw) r(c, rw) = (*this)(rw, c);
    return r;
}
float Mat2::determinant() const noexcept { return m[0]*m[3] - m[1]*m[2]; }
bool Mat2::invert() noexcept {
    float d = determinant();
    if (d == 0) return false;
    Mat2 r;
    r(0,0) = m[3]/d;  r(1,0) = -m[2]/d;
    r(0,1) = -m[1]/d; r(1,1) = m[0]/d;
    *this = r;
    return true;
}

Mat2 Mat2::rotation(float radians) noexcept {
    Mat2 r;
    float c = std::cos(radians), s = std::sin(radians);
    r(0,0)=c; r(0,1)=-s; r(1,0)=s; r(1,1)=c;
    return r;
}
Mat2 Mat2::scale(const TVec2<float>& s) noexcept {
    Mat2 r = Mat2::zero();
    r(0,0)=s.x; r(1,1)=s.y;
    return r;
}

// ---------------------------------------------------------------------------
// Mat3
// ---------------------------------------------------------------------------

Mat3::Mat3() noexcept { setIdentity(); }
Mat3::Mat3(float s) noexcept { std::memset(m, 0, sizeof(m)); m[0]=s; m[4]=s; m[8]=s; }

Mat3 Mat3::zero() noexcept { Mat3 r{}; std::memset(r.m, 0, sizeof(r.m)); return r; }
Mat3 Mat3::identity() noexcept { return Mat3(); }

void Mat3::setIdentity() noexcept { std::memset(m, 0, sizeof(m)); m[0]=1; m[4]=1; m[8]=1; }

float& Mat3::operator()(int col, int row) noexcept { return m[col * 3 + row]; }
const float& Mat3::operator()(int col, int row) const noexcept { return m[col * 3 + row]; }

Mat3 Mat3::operator*(const Mat3& b) const noexcept {
    Mat3 r = Mat3::zero();
    for (int c = 0; c < 3; ++c)
        for (int rw = 0; rw < 3; ++rw) {
            float sum = 0;
            for (int k = 0; k < 3; ++k) sum += (*this)(k, rw) * b(c, k);
            r(c, rw) = sum;
        }
    return r;
}
TVec3<float> Mat3::operator*(const TVec3<float>& v) const noexcept {
    return { m[0]*v.x + m[3]*v.y + m[6]*v.z,
             m[1]*v.x + m[4]*v.y + m[7]*v.z,
             m[2]*v.x + m[5]*v.y + m[8]*v.z };
}
Mat3 Mat3::transpose() const noexcept {
    Mat3 r;
    for (int c = 0; c < 3; ++c) for (int rw = 0; rw < 3; ++rw) r(c, rw) = (*this)(rw, c);
    return r;
}
float Mat3::determinant() const noexcept {
    return m[0]*(m[4]*m[8]-m[7]*m[5])
         - m[3]*(m[1]*m[8]-m[7]*m[2])
         + m[6]*(m[1]*m[5]-m[4]*m[2]);
}
Mat3 Mat3::adjugate() const noexcept {
    Mat3 r;
    r(0,0)= m[4]*m[8]-m[7]*m[5]; r(0,1)=-(m[3]*m[8]-m[6]*m[5]); r(0,2)= m[3]*m[7]-m[6]*m[4];
    r(1,0)=-(m[1]*m[8]-m[7]*m[2]); r(1,1)= m[0]*m[8]-m[6]*m[2]; r(1,2)=-(m[0]*m[7]-m[6]*m[1]);
    r(2,0)= m[1]*m[5]-m[4]*m[2]; r(2,1)=-(m[0]*m[5]-m[3]*m[2]); r(2,2)= m[0]*m[4]-m[3]*m[1];
    return r;
}
bool Mat3::invert() noexcept {
    float d = determinant();
    if (d == 0) return false;
    Mat3 adj = adjugate();
    for (int i = 0; i < 9; ++i) adj.m[i] /= d;
    *this = adj;
    return true;
}

Mat3 Mat3::rotationX(float radians) noexcept {
    Mat3 r; float c = std::cos(radians), s = std::sin(radians);
    r(1,1)=c; r(1,2)=-s; r(2,1)=s; r(2,2)=c; return r;
}
Mat3 Mat3::rotationY(float radians) noexcept {
    Mat3 r; float c = std::cos(radians), s = std::sin(radians);
    r(0,0)=c; r(0,2)=s; r(2,0)=-s; r(2,2)=c; return r;
}
Mat3 Mat3::rotationZ(float radians) noexcept {
    Mat3 r; float c = std::cos(radians), s = std::sin(radians);
    r(0,0)=c; r(0,1)=-s; r(1,0)=s; r(1,1)=c; return r;
}
Mat3 Mat3::scale(const TVec3<float>& s) noexcept {
    Mat3 r = Mat3::zero(); r(0,0)=s.x; r(1,1)=s.y; r(2,2)=s.z; return r;
}

// ---------------------------------------------------------------------------
// Mat4
// ---------------------------------------------------------------------------

Mat4::Mat4() noexcept { setIdentity(); }
Mat4::Mat4(float s) noexcept { std::memset(m, 0, sizeof(m)); m[0]=s; m[5]=s; m[10]=s; m[15]=s; }

Mat4 Mat4::zero() noexcept { Mat4 r{}; std::memset(r.m, 0, sizeof(r.m)); return r; }
Mat4 Mat4::identity() noexcept { return Mat4(); }

void Mat4::setIdentity() noexcept {
    std::memset(m, 0, sizeof(m));
    m[0]=1; m[5]=1; m[10]=1; m[15]=1;
}

float& Mat4::operator()(int col, int row) noexcept { return m[col * 4 + row]; }
const float& Mat4::operator()(int col, int row) const noexcept { return m[col * 4 + row]; }
const float* Mat4::data() const noexcept { return m; }
float* Mat4::data() noexcept { return m; }

Mat4 Mat4::operator*(const Mat4& b) const noexcept {
    Mat4 r = Mat4::zero();
    for (int c = 0; c < 4; ++c)
        for (int rw = 0; rw < 4; ++rw) {
            float sum = 0;
            for (int k = 0; k < 4; ++k) sum += (*this)(k, rw) * b(c, k);
            r(c, rw) = sum;
        }
    return r;
}
TVec4<float> Mat4::operator*(const TVec4<float>& v) const noexcept {
    return { m[0]*v.x + m[4]*v.y + m[8]*v.z  + m[12]*v.w,
             m[1]*v.x + m[5]*v.y + m[9]*v.z  + m[13]*v.w,
             m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w,
             m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w };
}
TVec3<float> Mat4::transformPoint(const TVec3<float>& v) const noexcept {
    TVec4<float> r = (*this) * TVec4<float>(v, 1.0f);
    float w = r.w;
    if (w != 0.0f && w != 1.0f) { r.x /= w; r.y /= w; r.z /= w; }
    return TVec3<float>(r.x, r.y, r.z);
}
TVec3<float> Mat4::transformVector(const TVec3<float>& v) const noexcept {
    TVec4<float> r = (*this) * TVec4<float>(v, 0.0f);
    return TVec3<float>(r.x, r.y, r.z);
}

Mat4 Mat4::transpose() const noexcept {
    Mat4 r;
    for (int c = 0; c < 4; ++c) for (int rw = 0; rw < 4; ++rw) r(c, rw) = (*this)(rw, c);
    return r;
}

Mat4 Mat4::translation(const TVec3<float>& t) noexcept {
    Mat4 r;
    r(3,0)=t.x; r(3,1)=t.y; r(3,2)=t.z;
    return r;
}
Mat4 Mat4::rotationX(float radians) noexcept {
    Mat4 r; float c=std::cos(radians), s=std::sin(radians);
    r(1,1)=c; r(1,2)=-s; r(2,1)=s; r(2,2)=c; return r;
}
Mat4 Mat4::rotationY(float radians) noexcept {
    Mat4 r; float c=std::cos(radians), s=std::sin(radians);
    r(0,0)=c; r(0,2)=s; r(2,0)=-s; r(2,2)=c; return r;
}
Mat4 Mat4::rotationZ(float radians) noexcept {
    Mat4 r; float c=std::cos(radians), s=std::sin(radians);
    r(0,0)=c; r(0,1)=-s; r(1,0)=s; r(1,1)=c; return r;
}
Mat4 Mat4::rotationAxis(const TVec3<float>& axis, float radians) noexcept {
    TVec3<float> a = axis.normalized();
    float c = std::cos(radians), s = std::sin(radians), t = 1 - c;
    Mat4 r;
    r(0,0)=t*a.x*a.x+c;       r(0,1)=t*a.x*a.y+s*a.z;   r(0,2)=t*a.x*a.z-s*a.y;
    r(1,0)=t*a.x*a.y-s*a.z;   r(1,1)=t*a.y*a.y+c;       r(1,2)=t*a.y*a.z+s*a.x;
    r(2,0)=t*a.x*a.z+s*a.y;   r(2,1)=t*a.y*a.z-s*a.x;   r(2,2)=t*a.z*a.z+c;
    return r;
}
Mat4 Mat4::scale(const TVec3<float>& s) noexcept {
    Mat4 r = Mat4::zero(); r(0,0)=s.x; r(1,1)=s.y; r(2,2)=s.z; r(3,3)=1; return r;
}

Mat4 Mat4::perspective(float fovYRadians, float aspect, float zNear, float zFar) noexcept {
    float f = 1.0f / std::tan(fovYRadians * 0.5f);
    Mat4 r = Mat4::zero();
    r(0,0)=f/aspect; r(1,1)=f;
    r(2,2)= (zFar+zNear)/(zNear-zFar);
    r(2,3)=-1;
    r(3,2)= 2*zNear*zFar/(zNear-zFar);
    return r;
}
Mat4 Mat4::orthographic(float left, float right, float bottom, float top,
                        float zNear, float zFar) noexcept {
    Mat4 r = Mat4::zero();
    r(0,0)=2/(right-left); r(1,1)=2/(top-bottom); r(2,2)=-2/(zFar-zNear);
    r(3,0)=-(right+left)/(right-left);
    r(3,1)=-(top+bottom)/(top-bottom);
    r(3,2)=-(zFar+zNear)/(zFar-zNear);
    r(3,3)=1;
    return r;
}

} // namespace math
} // namespace iml