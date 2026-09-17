#pragma once
/// @file
/// Practical 2D/3D geometric primitives and queries: shape types, containment,
/// intersection, distance, triangle and clipping helpers.

#include "imagelib/core/Types.h"
#include "imagelib/math/Scalar.h"
#include "imagelib/math/Vector.h"

#include <cmath>
#include <utility>
#include <vector>

namespace iml {
namespace math {

using Point2 = IVec2;    ///< Integral 2D point.
using RectI = iml::Rect; ///< Integral rect (from Types.h).

/// Floating rectangle.
struct RectF {
    /// Top-left corner (x, y) and size (width, height).
    float x = 0, y = 0, width = 0, height = 0;
    /// Default constructor; zero-initializes.
    constexpr RectF() noexcept = default;
    /// Constructs a rectangle from position and size.
    /// @param xi Left edge.
    /// @param yi Top edge.
    /// @param w Width.
    /// @param h Height.
    constexpr RectF(float xi, float yi, float w, float h) noexcept
        : x(xi), y(yi), width(w), height(h) {}
    /// Converts an integral rectangle.
    /// @param r Integral rectangle.
    explicit RectF(const iml::Rect& r) noexcept;

    /// Returns the left edge.
    /// @return Left edge coordinate.
    constexpr float left() const noexcept { return x; }
    /// Returns the top edge.
    /// @return Top edge coordinate.
    constexpr float top() const noexcept { return y; }
    /// Returns the right edge (x + width).
    /// @return Right edge coordinate.
    constexpr float right() const noexcept { return x + width; }
    /// Returns the bottom edge (y + height).
    /// @return Bottom edge coordinate.
    constexpr float bottom() const noexcept { return y + height; }
    /// Returns true when width or height is non-positive.
    /// @return True when the rectangle is empty.
    constexpr bool empty() const noexcept { return width <= 0 || height <= 0; }
    /// Tests whether p lies inside the rectangle (right/bottom exclusive).
    /// @param p Point to test.
    /// @return True when p is contained.
    constexpr bool contains(const Vec2& p) const noexcept {
        return p.x >= x && p.x < right() && p.y >= y && p.y < bottom();
    }
};

/// Axis-aligned bounding box (min..max), 2D.
struct AABB2 {
    /// Minimum corner.
    Vec2 min;
    /// Maximum corner.
    Vec2 max;

    /// Default constructor; zero-initializes both corners.
    constexpr AABB2() noexcept : min(0, 0), max(0, 0) {}
    /// Constructs from two corner points (order is not required).
    /// @param a First corner.
    /// @param b Second corner.
    AABB2(const Vec2& a, const Vec2& b) noexcept;

    /// Returns a box centered at center with the given extents.
    /// @param center Box center.
    /// @param radius Half extents per axis.
    /// @return Bounding box.
    static AABB2 fromCenterRadius(const Vec2& center, const Vec2& radius) noexcept;
    /// Returns the box center.
    /// @return Center point.
    Vec2 center() const noexcept;
    /// Returns the half extents.
    /// @return Radius per axis.
    Vec2 extent() const noexcept;
    /// Returns true when the box is valid (min <= max).
    /// @return True when the box is non-empty.
    bool isValid() const noexcept;
    /// Tests whether p lies inside the box (inclusive).
    /// @param p Point to test.
    /// @return True when p is contained.
    bool contains(const Vec2& p) const noexcept;
    /// Expands the box to include p.
    /// @param p Point to include.
    void grow(const Vec2& p) noexcept;
    /// Tests overlap with o.
    /// @param o Other box.
    /// @return True when the boxes overlap.
    bool overlaps(const AABB2& o) const noexcept;
};

/// Axis-aligned bounding box, 3D.
struct AABB3 {
    /// Minimum corner.
    Vec3 min;
    /// Maximum corner.
    Vec3 max;

    /// Default constructor; zero-initializes both corners.
    constexpr AABB3() noexcept : min(0, 0, 0), max(0, 0, 0) {}
    /// Constructs from two corner points (order is not required).
    /// @param a First corner.
    /// @param b Second corner.
    AABB3(const Vec3& a, const Vec3& b) noexcept;
    /// Returns the box center.
    /// @return Center point.
    Vec3 center() const noexcept;
    /// Tests whether p lies inside the box (inclusive).
    /// @param p Point to test.
    /// @return True when p is contained.
    bool contains(const Vec3& p) const noexcept;
    /// Expands the box to include p.
    /// @param p Point to include.
    void grow(const Vec3& p) noexcept;
    /// Tests overlap with o.
    /// @param o Other box.
    /// @return True when the boxes overlap.
    bool overlaps(const AABB3& o) const noexcept;
};

/// 2D circle.
struct Circle {
    /// Center point.
    Vec2 center;
    /// Radius.
    float radius = 0;

    /// Default constructor; zero-initializes.
    constexpr Circle() noexcept = default;
    /// Constructs a circle from a center and radius.
    /// @param c Center point.
    /// @param r Radius.
    constexpr Circle(const Vec2& c, float r) noexcept : center(c), radius(r) {}
    /// Tests whether p lies inside the circle.
    /// @param p Point to test.
    /// @return True when p is contained.
    bool contains(const Vec2& p) const noexcept;
};

/// 3D sphere.
struct Sphere {
    /// Center point.
    Vec3 center;
    /// Radius.
    float radius = 0;

    /// Default constructor; zero-initializes.
    constexpr Sphere() noexcept = default;
    /// Constructs a sphere from a center and radius.
    /// @param c Center point.
    /// @param r Radius.
    constexpr Sphere(const Vec3& c, float r) noexcept : center(c), radius(r) {}
    /// Tests whether p lies inside the sphere.
    /// @param p Point to test.
    /// @return True when p is contained.
    bool contains(const Vec3& p) const noexcept;
};

/// Line through two points (infinite).
struct Line2 {
    /// First point.
    Vec2 a;
    /// Second point.
    Vec2 b;
};

/// Finite segment (clamped between endpoints).
using Segment2 = Line2;

/// 3D ray.
struct Ray {
    /// Origin of the ray (t = 0).
    Vec3 origin;
    /// Unit direction; normalized in the constructor.
    Vec3 direction;
    /// Default constructor; zero-initializes.
    Ray() noexcept = default;
    /// Constructs a ray and normalizes direction.
    /// @param o Origin.
    /// @param d Direction.
    Ray(const Vec3& o, const Vec3& d) noexcept;
    /// Returns the point at parameter t along the ray.
    /// @param t Ray parameter.
    /// @return origin + t * direction.
    Vec3 pointAt(float t) const noexcept;
};

/// 3D plane: n . p + d == 0, n normalized.
struct Plane {
    /// Unit normal.
    Vec3 normal;
    /// Plane offset.
    float d = 0;

    /// Default constructor; zero-initializes.
    Plane() noexcept = default;
    /// Constructs a plane from a normal and offset.
    /// @param n Unit normal.
    /// @param dval Offset.
    Plane(const Vec3& n, float dval) noexcept;
    /// Plane from a triangle (counter-clockwise winding convention).
    /// @param a First vertex.
    /// @param b Second vertex.
    /// @param c Third vertex.
    /// @return Plane through the triangle.
    static Plane fromTriangle(const Vec3& a, const Vec3& b, const Vec3& c);
    /// Signed distance from p to the plane.
    /// @param p Point.
    /// @return Signed distance.
    float signedDistance(const Vec3& p) const noexcept;
    /// Absolute distance from p to the plane.
    /// @param p Point.
    /// @return Distance.
    float distance(const Vec3& p) const noexcept;
};

/// 2D triangle.
struct Triangle2 {
    /// Vertices a, b and c.
    Vec2 a, b, c;
};

/// 3D triangle.
struct Triangle3 {
    /// Vertices a, b and c.
    Vec3 a, b, c;
};

/// Tests whether p lies inside the integral rectangle r.
/// @param p Point to test.
/// @param r Rectangle.
/// @return True when p is contained.
bool pointInRect(const Point2& p, const RectI& r) noexcept;

/// Tests whether two integral rectangles overlap.
/// @param a First rectangle.
/// @param b Second rectangle.
/// @return True when the rectangles overlap.
bool rectsOverlap(const RectI& a, const RectI& b) noexcept;

/// Tests whether the circle contains p.
/// @param c Circle.
/// @param p Point to test.
/// @param eps Distance epsilon.
/// @return True when p is contained.
bool circleContainsPoint(const Circle& c, const Vec2& p, float eps = 0.0f) noexcept;

/// Tests whether two circles overlap.
/// @param a First circle.
/// @param b Second circle.
/// @return True when the circles overlap.
bool circlesOverlap(const Circle& a, const Circle& b) noexcept;

/// Tests whether the circle fully contains the rectangle.
/// @param c Circle.
/// @param r Rectangle.
/// @return True when the rectangle is contained.
bool circleContainsRect(const Circle& c, const RectF& r) noexcept;

/// Tests whether the circle overlaps the rectangle.
/// @param c Circle.
/// @param r Rectangle.
/// @return True when the shapes overlap.
bool circleOverlapsRect(const Circle& c, const RectF& r) noexcept;

/// Tests whether the sphere contains p.
/// @param s Sphere.
/// @param p Point to test.
/// @return True when p is contained.
bool sphereContainsPoint(const Sphere& s, const Vec3& p) noexcept;

/// Intersects a ray with a sphere; writes the hit parameter.
/// @param ray Ray.
/// @param s Sphere.
/// @param tOut Hit parameter along the ray.
/// @return True on intersection.
bool rayIntersectsSphere(const Ray& ray, const Sphere& s, float& tOut) noexcept;

/// Intersects a ray with a plane; writes the hit parameter.
/// @param ray Ray.
/// @param plane Plane.
/// @param tOut Hit parameter along the ray.
/// @return True on intersection.
bool rayIntersectsPlane(const Ray& ray, const Plane& plane, float& tOut) noexcept;

/// Intersects a ray with a box; writes the entry and exit parameters.
/// @param ray Ray.
/// @param box Box.
/// @param tMin Entry parameter.
/// @param tMax Exit parameter.
/// @return True on intersection.
bool rayIntersectsAABB(const Ray& ray, const AABB3& box, float& tMin, float& tMax) noexcept;

/// Closest point on an infinite line (a + (b-a)*t).
/// @param p Query point.
/// @param line Infinite line.
/// @return Closest point.
Vec2 closestPointOnLine(const Vec2& p, const Line2& line) noexcept;

/// Closest point on a finite segment.
/// @param p Query point.
/// @param seg Segment.
/// @return Closest point.
Vec2 closestPointOnSegment(const Vec2& p, const Segment2& seg) noexcept;

/// Distance from p to a segment.
/// @param p Query point.
/// @param seg Segment.
/// @return Distance.
float distanceToSegment(const Vec2& p, const Segment2& seg) noexcept;

/// Distance from p to an infinite line.
/// @param p Query point.
/// @param line Infinite line.
/// @return Distance.
float distanceToLine(const Vec2& p, const Line2& line) noexcept;

/// Closest point on a plane to p.
/// @param p Query point.
/// @param plane Plane.
/// @return Closest point.
Vec3 closestPointOnPlane(const Vec3& p, const Plane& plane) noexcept;

/// Distance from p to a plane.
/// @param p Query point.
/// @param plane Plane.
/// @return Distance.
float distanceToPlane(const Vec3& p, const Plane& plane) noexcept;

/// Closest point on segment ab to p (3D).
/// @param p Query point.
/// @param a Segment start.
/// @param b Segment end.
/// @return Closest point.
Vec3 closestPointOnSegment3(const Vec3& p, const Vec3& a, const Vec3& b) noexcept;

/// Closest point on a triangle to p (Ericson, "Real-Time Collision Detection").
/// @param p Query point.
/// @param tri Triangle.
/// @return Closest point.
Vec3 closestPointOnTriangle(const Vec3& p, const Triangle3& tri) noexcept;

/// Signed area of a 2D triangle.
/// @param t Triangle.
/// @return Signed area.
float triangleArea2D(const Triangle2& t) noexcept;

/// Centroid of a 2D triangle.
/// @param t Triangle.
/// @return Centroid point.
Vec2 triangleCentroid2D(const Triangle2& t) noexcept;

/// Barycentric coordinates (u, v, w) with p == u*a + v*b + w*c.
/// @param p Point.
/// @param tri Triangle.
/// @return Barycentric coordinate triple.
Vec3 barycentric(const Vec2& p, const Triangle2& tri) noexcept;

/// Tests whether a 2D triangle contains p.
/// @param p Point to test.
/// @param tri Triangle.
/// @return True when p is contained.
bool triangleContains2D(const Vec2& p, const Triangle2& tri) noexcept;

/// Liang-Barsky: clips segment [a,b] to the rect; writes clipped endpoints.
/// Returns false when fully outside.
/// @param a Segment start.
/// @param b Segment end.
/// @param r Clipping rectangle.
/// @param outA Clipped start.
/// @param outB Clipped end.
/// @return True when the segment survives clipping.
bool clipSegment(const Vec2& a, const Vec2& b, const RectF& r, Vec2& outA, Vec2& outB) noexcept;

/// Sutherland-Hodgman: clips a convex polygon to a rect. `in` is a list of
/// vertices; returns the clipped vertex list (may be empty).
/// @param in Input polygon vertices.
/// @param r Clipping rectangle.
/// @return Clipped polygon vertices.
/// @throws std::bad_alloc if allocating the result fails.
std::vector<Vec2> clipPolygonToRect(const std::vector<Vec2>& in, const RectF& r);

} // namespace math
} // namespace iml