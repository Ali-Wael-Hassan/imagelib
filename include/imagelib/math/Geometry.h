#pragma once
// imagelib/math/Geometry.h
//
// Practical 2D/3D geometric primitives and queries:
//   points, rectangles, AABBs, circles/spheres, lines/segments/rays, planes,
//   triangles, plus containment / intersection / distance / clipping helpers.
//
// constexpr / defaulted members live in the headers; non-template bodies are
// compiled out-of-line in src/math/Geometry.cpp.

#include "imagelib/core/Types.h"
#include "imagelib/math/Scalar.h"
#include "imagelib/math/Vector.h"

#include <cmath>
#include <utility>
#include <vector>

namespace iml {
namespace math {

// ---------------------------------------------------------------------------
// Basic shape types
// ---------------------------------------------------------------------------

using Point2 = IVec2;           ///< Integral 2D point.
using RectI  = iml::Rect;       ///< Integral rect (from Types.h).

/// Floating rectangle.
struct RectF {
    float x = 0, y = 0, width = 0, height = 0;
    constexpr RectF() noexcept = default;
    constexpr RectF(float xi, float yi, float w, float h) noexcept : x(xi), y(yi), width(w), height(h) {}
    explicit RectF(const iml::Rect& r) noexcept;

    constexpr float left()   const noexcept { return x; }
    constexpr float top()    const noexcept { return y; }
    constexpr float right()  const noexcept { return x + width; }
    constexpr float bottom() const noexcept { return y + height; }
    constexpr bool  empty()  const noexcept { return width <= 0 || height <= 0; }
    constexpr bool  contains(const Vec2& p) const noexcept {
        return p.x >= x && p.x < right() && p.y >= y && p.y < bottom();
    }
};

/// Axis-aligned bounding box (min..max), 2D.
struct AABB2 {
    Vec2 min;
    Vec2 max;

    constexpr AABB2() noexcept : min(0, 0), max(0, 0) {}
    AABB2(const Vec2& a, const Vec2& b) noexcept;

    static AABB2 fromCenterRadius(const Vec2& center, const Vec2& radius) noexcept;
    Vec2 center() const noexcept;
    Vec2 extent() const noexcept;
    bool isValid() const noexcept;
    bool contains(const Vec2& p) const noexcept;
    void grow(const Vec2& p) noexcept;
    bool overlaps(const AABB2& o) const noexcept;
};

/// Axis-aligned bounding box, 3D.
struct AABB3 {
    Vec3 min;
    Vec3 max;

    constexpr AABB3() noexcept : min(0, 0, 0), max(0, 0, 0) {}
    AABB3(const Vec3& a, const Vec3& b) noexcept;
    Vec3 center() const noexcept;
    bool contains(const Vec3& p) const noexcept;
    void grow(const Vec3& p) noexcept;
    bool overlaps(const AABB3& o) const noexcept;
};

/// 2D circle.
struct Circle {
    Vec2  center;
    float radius = 0;

    constexpr Circle() noexcept = default;
    constexpr Circle(const Vec2& c, float r) noexcept : center(c), radius(r) {}
    bool contains(const Vec2& p) const noexcept;
};

/// 3D sphere.
struct Sphere {
    Vec3  center;
    float radius = 0;

    constexpr Sphere() noexcept = default;
    constexpr Sphere(const Vec3& c, float r) noexcept : center(c), radius(r) {}
    bool contains(const Vec3& p) const noexcept;
};

/// Line through two points (infinite).
struct Line2 {
    Vec2 a;
    Vec2 b;
};

/// Finite segment (clamped between endpoints).
using Segment2 = Line2;

/// 3D ray.
struct Ray {
    Vec3 origin;      //< t = 0
    Vec3 direction;   //< unit direction implied; normalized in ctor
    Ray() noexcept = default;
    Ray(const Vec3& o, const Vec3& d) noexcept;
    Vec3 pointAt(float t) const noexcept;
};

/// 3D plane: n . p + d == 0, n normalized.
struct Plane {
    Vec3  normal;
    float d = 0;

    Plane() noexcept = default;
    Plane(const Vec3& n, float dval) noexcept;
    /// Plane from a triangle (counter-clockwise winding convention).
    static Plane fromTriangle(const Vec3& a, const Vec3& b, const Vec3& c);
    float signedDistance(const Vec3& p) const noexcept;
    float distance(const Vec3& p) const noexcept;
};

/// 2D triangle.
struct Triangle2 {
    Vec2 a, b, c;
};

/// 3D triangle.
struct Triangle3 {
    Vec3 a, b, c;
};

// ---------------------------------------------------------------------------
// Containment / intersection queries
// ---------------------------------------------------------------------------

bool pointInRect(const Point2& p, const RectI& r) noexcept;

bool rectsOverlap(const RectI& a, const RectI& b) noexcept;

bool circleContainsPoint(const Circle& c, const Vec2& p, float eps = 0.0f) noexcept;

bool circlesOverlap(const Circle& a, const Circle& b) noexcept;

bool circleContainsRect(const Circle& c, const RectF& r) noexcept;

bool circleOverlapsRect(const Circle& c, const RectF& r) noexcept;

bool sphereContainsPoint(const Sphere& s, const Vec3& p) noexcept;

bool rayIntersectsSphere(const Ray& ray, const Sphere& s, float& tOut) noexcept;

bool rayIntersectsPlane(const Ray& ray, const Plane& plane, float& tOut) noexcept;

bool rayIntersectsAABB(const Ray& ray, const AABB3& box, float& tMin, float& tMax) noexcept;

// ---------------------------------------------------------------------------
// Distances / closest points
// ---------------------------------------------------------------------------

/// Closest point on an infinite line (a + (b-a)*t).
Vec2 closestPointOnLine(const Vec2& p, const Line2& line) noexcept;

Vec2 closestPointOnSegment(const Vec2& p, const Segment2& seg) noexcept;

float distanceToSegment(const Vec2& p, const Segment2& seg) noexcept;

float distanceToLine(const Vec2& p, const Line2& line) noexcept;

Vec3 closestPointOnPlane(const Vec3& p, const Plane& plane) noexcept;

float distanceToPlane(const Vec3& p, const Plane& plane) noexcept;

Vec3 closestPointOnSegment3(const Vec3& p, const Vec3& a, const Vec3& b) noexcept;

/// Closest point on a triangle to p (Ericson, "Real-Time Collision Detection").
Vec3 closestPointOnTriangle(const Vec3& p, const Triangle3& tri) noexcept;

// ---------------------------------------------------------------------------
// Triangles
// ---------------------------------------------------------------------------

float triangleArea2D(const Triangle2& t) noexcept;

Vec2 triangleCentroid2D(const Triangle2& t) noexcept;

/// Barycentric coordinates (u, v, w) with p == u*a + v*b + w*c.
Vec3 barycentric(const Vec2& p, const Triangle2& tri) noexcept;

bool triangleContains2D(const Vec2& p, const Triangle2& tri) noexcept;

// ---------------------------------------------------------------------------
// Clipping
// ---------------------------------------------------------------------------

/// Liang-Barsky: clips segment [a,b] to the rect; writes clipped endpoints.
/// Returns false when fully outside.
bool clipSegment(const Vec2& a, const Vec2& b, const RectF& r,
                 Vec2& outA, Vec2& outB) noexcept;

/// Sutherland-Hodgman: clips a convex polygon to a rect. `in` is a list of
/// vertices; returns the clipped vertex list (may be empty).
std::vector<Vec2> clipPolygonToRect(const std::vector<Vec2>& in, const RectF& r);

} // namespace math
} // namespace iml