#pragma once
// imagelib/math/Matrix.h
//
// Mat2/Mat3/Mat4, column-major storage (GLM convention):
//   m(row, col) == data[col * N + row]
// Column vectors: M * v transforms v; translation lives in the last column.
//
// Non-template matrix math is compiled out-of-line in src/math/Matrix.cpp.
// Quaternion bridge functions also live in src/math/Matrix.cpp to avoid an
// include cycle.

#include "imagelib/core/Types.h"
#include "imagelib/math/Scalar.h"
#include "imagelib/math/Vector.h"

#include <cstring>
#include <cmath>

namespace iml {
namespace math {

struct Quat; // fwd: complete in Quaternion.h

// ---------------------------------------------------------------------------
// Mat2
// ---------------------------------------------------------------------------
struct Mat2 {
    float m[4]; // column-major

    Mat2() noexcept;
    explicit Mat2(float s) noexcept;

    static Mat2 zero() noexcept;
    static Mat2 identity() noexcept;

    void setIdentity() noexcept;

    float& operator()(int col, int row) noexcept;
    const float& operator()(int col, int row) const noexcept;

    Mat2 operator*(const Mat2& b) const noexcept;
    TVec2<float> operator*(const TVec2<float>& v) const noexcept;
    Mat2 transpose() const noexcept;
    float determinant() const noexcept;
    bool invert() noexcept;

    static Mat2 rotation(float radians) noexcept;
    static Mat2 scale(const TVec2<float>& s) noexcept;
};

// ---------------------------------------------------------------------------
// Mat3
// ---------------------------------------------------------------------------
struct Mat3 {
    float m[9]; // column-major

    Mat3() noexcept;
    explicit Mat3(float s) noexcept;

    static Mat3 zero() noexcept;
    static Mat3 identity() noexcept;

    void setIdentity() noexcept;

    float& operator()(int col, int row) noexcept;
    const float& operator()(int col, int row) const noexcept;

    Mat3 operator*(const Mat3& b) const noexcept;
    TVec3<float> operator*(const TVec3<float>& v) const noexcept;
    Mat3 transpose() const noexcept;
    float determinant() const noexcept;
    Mat3 adjugate() const noexcept;
    bool invert() noexcept;

    static Mat3 rotationX(float radians) noexcept;
    static Mat3 rotationY(float radians) noexcept;
    static Mat3 rotationZ(float radians) noexcept;
    static Mat3 scale(const TVec3<float>& s) noexcept;
};

// ---------------------------------------------------------------------------
// Mat4
// ---------------------------------------------------------------------------
struct Mat4 {
    float m[16]; // column-major

    Mat4() noexcept;
    explicit Mat4(float s) noexcept;

    static Mat4 zero() noexcept;
    static Mat4 identity() noexcept;

    void setIdentity() noexcept;

    float& operator()(int col, int row) noexcept;
    const float& operator()(int col, int row) const noexcept;
    const float* data() const noexcept;
    float* data() noexcept;

    Mat4 operator*(const Mat4& b) const noexcept;
    TVec4<float> operator*(const TVec4<float>& v) const noexcept;
    TVec3<float> transformPoint(const TVec3<float>& v) const noexcept;
    TVec3<float> transformVector(const TVec3<float>& v) const noexcept;

    Mat4 transpose() const noexcept;
    float determinant() const noexcept;
    Mat4 adjugate() const noexcept;
    /// Returns inverse; if |det| < epsilon, returns the adjugate scaled to 1.
    Mat4 inverse() const noexcept;

    static Mat4 translation(const TVec3<float>& t) noexcept;
    static Mat4 rotationX(float radians) noexcept;
    static Mat4 rotationY(float radians) noexcept;
    static Mat4 rotationZ(float radians) noexcept;
    static Mat4 rotationAxis(const TVec3<float>& axis, float radians) noexcept;
    static Mat4 fromQuaternion(const Quat& q) noexcept;
    static Mat4 scale(const TVec3<float>& s) noexcept;
    static Mat4 compose(const TVec3<float>& t, const Quat& r, const TVec3<float>& s) noexcept;

    static Mat4 perspective(float fovYRadians, float aspect, float zNear, float zFar) noexcept;
    static Mat4 orthographic(float left, float right, float bottom, float top,
                             float zNear, float zFar) noexcept;
    static Mat4 lookAt(const TVec3<float>& eye, const TVec3<float>& center,
                       const TVec3<float>& up) noexcept;
};

} // namespace math
} // namespace iml