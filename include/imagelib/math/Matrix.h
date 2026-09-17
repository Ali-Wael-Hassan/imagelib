#pragma once
/// @file
/// 2D/3D/4D matrices with column-major storage (GLM convention).

#include "imagelib/core/Types.h"
#include "imagelib/math/Scalar.h"
#include "imagelib/math/Vector.h"

#include <cstring>
#include <cmath>

namespace iml {
namespace math {

/// Forward declaration; defined in Quaternion.h.
struct Quat;

/// 2x2 column-major matrix.
struct Mat2 {
    /// Column-major storage of 4 elements.
    float m[4];

    /// Default constructor.
    Mat2() noexcept;
    /// Constructs a matrix with s on the diagonal.
    /// @param s Diagonal value.
    explicit Mat2(float s) noexcept;

    /// Returns a matrix with all elements zero.
    /// @return Zero matrix.
    static Mat2 zero() noexcept;
    /// Returns the identity matrix.
    /// @return Identity matrix.
    static Mat2 identity() noexcept;

    /// Sets this to the identity matrix.
    void setIdentity() noexcept;

    /// Returns a reference to the element at (col, row).
    /// @param col Column index.
    /// @param row Row index.
    /// @return Reference to the element.
    float& operator()(int col, int row) noexcept;
    /// Returns the element at (col, row).
    /// @param col Column index.
    /// @param row Row index.
    /// @return Element value.
    const float& operator()(int col, int row) const noexcept;

    /// Returns the matrix product this * b.
    /// @param b Right-hand operand.
    /// @return Product matrix.
    Mat2 operator*(const Mat2& b) const noexcept;
    /// Transforms the column vector v.
    /// @param v Vector to transform.
    /// @return Transformed vector.
    TVec2<float> operator*(const TVec2<float>& v) const noexcept;
    /// Returns the transpose.
    /// @return Transposed matrix.
    Mat2 transpose() const noexcept;
    /// Returns the determinant.
    /// @return Determinant.
    float determinant() const noexcept;
    /// Inverts this in place.
    /// @return True when the matrix is invertible.
    bool invert() noexcept;

    /// Returns a counter-clockwise rotation matrix.
    /// @param radians Rotation angle in radians.
    /// @return Rotation matrix.
    static Mat2 rotation(float radians) noexcept;
    /// Returns a scale matrix.
    /// @param s Per-axis scale.
    /// @return Scale matrix.
    static Mat2 scale(const TVec2<float>& s) noexcept;
};

/// 3x3 column-major matrix.
struct Mat3 {
    /// Column-major storage of 9 elements.
    float m[9];

    /// Default constructor.
    Mat3() noexcept;
    /// Constructs a matrix with s on the diagonal.
    /// @param s Diagonal value.
    explicit Mat3(float s) noexcept;

    /// Returns a matrix with all elements zero.
    /// @return Zero matrix.
    static Mat3 zero() noexcept;
    /// Returns the identity matrix.
    /// @return Identity matrix.
    static Mat3 identity() noexcept;

    /// Sets this to the identity matrix.
    void setIdentity() noexcept;

    /// Returns a reference to the element at (col, row).
    /// @param col Column index.
    /// @param row Row index.
    /// @return Reference to the element.
    float& operator()(int col, int row) noexcept;
    /// Returns the element at (col, row).
    /// @param col Column index.
    /// @param row Row index.
    /// @return Element value.
    const float& operator()(int col, int row) const noexcept;

    /// Returns the matrix product this * b.
    /// @param b Right-hand operand.
    /// @return Product matrix.
    Mat3 operator*(const Mat3& b) const noexcept;
    /// Transforms the column vector v.
    /// @param v Vector to transform.
    /// @return Transformed vector.
    TVec3<float> operator*(const TVec3<float>& v) const noexcept;
    /// Returns the transpose.
    /// @return Transposed matrix.
    Mat3 transpose() const noexcept;
    /// Returns the determinant.
    /// @return Determinant.
    float determinant() const noexcept;
    /// Returns the adjugate (transpose of the cofactor matrix).
    /// @return Adjugate matrix.
    Mat3 adjugate() const noexcept;
    /// Inverts this in place.
    /// @return True when the matrix is invertible.
    bool invert() noexcept;

    /// Returns a rotation matrix about the X axis.
    /// @param radians Rotation angle in radians.
    /// @return Rotation matrix.
    static Mat3 rotationX(float radians) noexcept;
    /// Returns a rotation matrix about the Y axis.
    /// @param radians Rotation angle in radians.
    /// @return Rotation matrix.
    static Mat3 rotationY(float radians) noexcept;
    /// Returns a rotation matrix about the Z axis.
    /// @param radians Rotation angle in radians.
    /// @return Rotation matrix.
    static Mat3 rotationZ(float radians) noexcept;
    /// Returns a scale matrix.
    /// @param s Per-axis scale.
    /// @return Scale matrix.
    static Mat3 scale(const TVec3<float>& s) noexcept;
};

/// 4x4 column-major matrix.
struct Mat4 {
    /// Column-major storage of 16 elements.
    float m[16];

    /// Default constructor.
    Mat4() noexcept;
    /// Constructs a matrix with s on the diagonal.
    /// @param s Diagonal value.
    explicit Mat4(float s) noexcept;

    /// Returns a matrix with all elements zero.
    /// @return Zero matrix.
    static Mat4 zero() noexcept;
    /// Returns the identity matrix.
    /// @return Identity matrix.
    static Mat4 identity() noexcept;

    /// Sets this to the identity matrix.
    void setIdentity() noexcept;

    /// Returns a reference to the element at (col, row).
    /// @param col Column index.
    /// @param row Row index.
    /// @return Reference to the element.
    float& operator()(int col, int row) noexcept;
    /// Returns the element at (col, row).
    /// @param col Column index.
    /// @param row Row index.
    /// @return Element value.
    const float& operator()(int col, int row) const noexcept;
    /// Returns a pointer to the raw column-major storage.
    /// @return Const pointer to m.
    const float* data() const noexcept;
    /// Returns a pointer to the raw column-major storage.
    /// @return Mutable pointer to m.
    float* data() noexcept;

    /// Returns the matrix product this * b.
    /// @param b Right-hand operand.
    /// @return Product matrix.
    Mat4 operator*(const Mat4& b) const noexcept;
    /// Transforms the column vector v.
    /// @param v Vector to transform.
    /// @return Transformed vector.
    TVec4<float> operator*(const TVec4<float>& v) const noexcept;
    /// Transforms a point (v, 1) and returns the projected xyz.
    /// @param v Point to transform.
    /// @return Transformed point.
    TVec3<float> transformPoint(const TVec3<float>& v) const noexcept;
    /// Transforms a direction (v, 0), ignoring translation.
    /// @param v Direction to transform.
    /// @return Transformed direction.
    TVec3<float> transformVector(const TVec3<float>& v) const noexcept;

    /// Returns the transpose.
    /// @return Transposed matrix.
    Mat4 transpose() const noexcept;
    /// Returns the determinant.
    /// @return Determinant.
    float determinant() const noexcept;
    /// Returns the adjugate (transpose of the cofactor matrix).
    /// @return Adjugate matrix.
    Mat4 adjugate() const noexcept;
    /// Returns inverse; if |det| < epsilon, returns the adjugate scaled to 1.
    /// @return Inverse matrix.
    Mat4 inverse() const noexcept;

    /// Returns a translation matrix.
    /// @param t Translation vector.
    /// @return Translation matrix.
    static Mat4 translation(const TVec3<float>& t) noexcept;
    /// Returns a rotation matrix about the X axis.
    /// @param radians Rotation angle in radians.
    /// @return Rotation matrix.
    static Mat4 rotationX(float radians) noexcept;
    /// Returns a rotation matrix about the Y axis.
    /// @param radians Rotation angle in radians.
    /// @return Rotation matrix.
    static Mat4 rotationY(float radians) noexcept;
    /// Returns a rotation matrix about the Z axis.
    /// @param radians Rotation angle in radians.
    /// @return Rotation matrix.
    static Mat4 rotationZ(float radians) noexcept;
    /// Returns a rotation matrix about an arbitrary axis.
    /// @param axis Rotation axis.
    /// @param radians Rotation angle in radians.
    /// @return Rotation matrix.
    static Mat4 rotationAxis(const TVec3<float>& axis, float radians) noexcept;
    /// Returns a rotation matrix from a quaternion.
    /// @param q Unit quaternion.
    /// @return Rotation matrix.
    static Mat4 fromQuaternion(const Quat& q) noexcept;
    /// Returns a scale matrix.
    /// @param s Per-axis scale.
    /// @return Scale matrix.
    static Mat4 scale(const TVec3<float>& s) noexcept;
    /// Composes translation, rotation and scale into one matrix.
    /// @param t Translation vector.
    /// @param r Unit quaternion.
    /// @param s Per-axis scale.
    /// @return Composed matrix.
    static Mat4 compose(const TVec3<float>& t, const Quat& r, const TVec3<float>& s) noexcept;

    /// Returns a perspective projection matrix.
    /// @param fovYRadians Vertical field of view in radians.
    /// @param aspect Aspect ratio (width / height).
    /// @param zNear Near clip distance.
    /// @param zFar Far clip distance.
    /// @return Perspective projection matrix.
    static Mat4 perspective(float fovYRadians, float aspect, float zNear, float zFar) noexcept;
    /// Returns an orthographic projection matrix.
    /// @param left Left clip plane.
    /// @param right Right clip plane.
    /// @param bottom Bottom clip plane.
    /// @param top Top clip plane.
    /// @param zNear Near clip distance.
    /// @param zFar Far clip distance.
    /// @return Orthographic projection matrix.
    static Mat4 orthographic(
        float left,
        float right,
        float bottom,
        float top,
        float zNear,
        float zFar) noexcept;
    /// Returns a view matrix looking from eye towards center.
    /// @param eye Camera position.
    /// @param center Look-at target.
    /// @param up Up direction.
    /// @return View matrix.
    static Mat4
    lookAt(const TVec3<float>& eye, const TVec3<float>& center, const TVec3<float>& up) noexcept;
};

} // namespace math
} // namespace iml