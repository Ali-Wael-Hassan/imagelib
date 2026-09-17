#include "imagelib/math/Geometry.h"

#include <cmath>
#include <vector>

namespace iml {
namespace math {

/// Constructs a float rect from its integer counterpart.
/// @param r Integer rect.
RectF::RectF(const iml::Rect& r) noexcept
    : x(static_cast<float>(r.x)), y(static_cast<float>(r.y)), width(static_cast<float>(r.width)),
      height(static_cast<float>(r.height)) {}

/// Constructs an AABB2 from the min corner a and max corner b.
/// @param a Minimum corner.
/// @param b Maximum corner.
AABB2::AABB2(const Vec2& a, const Vec2& b) noexcept : min(a), max(b) {}

/// Builds an AABB2 from a center and half-extent radius.
/// @param center Box center.
/// @param radius Half-extent per axis.
/// @return The box.
AABB2 AABB2::fromCenterRadius(const Vec2& center, const Vec2& radius) noexcept {
    return AABB2(center - radius, center + radius);
}
/// Returns the box center.
/// @return The center.
Vec2 AABB2::center() const noexcept { return (min + max) * 0.5f; }
/// Returns the box extent (max - min).
/// @return The extent.
Vec2 AABB2::extent() const noexcept { return max - min; }
/// True when the box is valid (min <= max on every axis).
/// @return Validity flag.
bool AABB2::isValid() const noexcept { return min.x <= max.x && min.y <= max.y; }
/// True when point p lies inside this box (inclusive).
/// @param p Point to test.
/// @return True when inside.
bool AABB2::contains(const Vec2& p) const noexcept {
    return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
}
/// Expands the box to include point p.
/// @param p Point to include.
void AABB2::grow(const Vec2& p) noexcept {
    min = minElementWise(min, p);
    max = maxElementWise(max, p);
}
/// True when this box overlaps box o.
/// @param o Other box.
/// @return True when overlapping.
bool AABB2::overlaps(const AABB2& o) const noexcept {
    return min.x <= o.max.x && max.x >= o.min.x && min.y <= o.max.y && max.y >= o.min.y;
}

/// Constructs an AABB3 from the min corner a and max corner b.
/// @param a Minimum corner.
/// @param b Maximum corner.
AABB3::AABB3(const Vec3& a, const Vec3& b) noexcept : min(a), max(b) {}
/// Returns the box center.
/// @return The center.
Vec3 AABB3::center() const noexcept { return (min + max) * 0.5f; }
/// True when point p lies inside this box (inclusive).
/// @param p Point to test.
/// @return True when inside.
bool AABB3::contains(const Vec3& p) const noexcept {
    return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y && p.z >= min.z &&
           p.z <= max.z;
}
/// Expands the box to include point p.
/// @param p Point to include.
void AABB3::grow(const Vec3& p) noexcept {
    min = minElementWise(min, p);
    max = maxElementWise(max, p);
}
/// True when this box overlaps box o.
/// @param o Other box.
/// @return True when overlapping.
bool AABB3::overlaps(const AABB3& o) const noexcept {
    return min.x <= o.max.x && max.x >= o.min.x && min.y <= o.max.y && max.y >= o.min.y &&
           min.z <= o.max.z && max.z >= o.min.z;
}

/// True when point p lies inside or on the circle.
/// @param p Point to test.
/// @return True when inside.
bool Circle::contains(const Vec2& p) const noexcept {
    return (p - center).squaredLength() <= radius * radius;
}

/// True when point p lies inside or on the sphere.
/// @param p Point to test.
/// @return True when inside.
bool Sphere::contains(const Vec3& p) const noexcept {
    return (p - center).squaredLength() <= radius * radius;
}

/// Constructs a ray from an origin and a direction.
/// @param o Ray origin.
/// @param d Direction; normalized on construction.
Ray::Ray(const Vec3& o, const Vec3& d) noexcept : origin(o) { direction = d.normalized(); }
/// Returns the point at distance t along the ray.
/// @param t Travel distance.
/// @return origin + direction * t.
Vec3 Ray::pointAt(float t) const noexcept { return origin + direction * t; }

/// Constructs a plane from a normal and offset d.
/// @param n Plane normal; normalized on construction.
/// @param dval Plane offset along the normal.
Plane::Plane(const Vec3& n, float dval) noexcept : normal(n.normalized()), d(dval) {}
/// Builds a plane passing through three points.
/// @param a First point.
/// @param b Second point.
/// @param c Third point.
/// @return The plane.
Plane Plane::fromTriangle(const Vec3& a, const Vec3& b, const Vec3& c) {
    Vec3 n = (b - a).cross(c - a).normalized();
    return Plane(n, -n.dot(a));
}
/// Returns the signed distance from point p to the plane.
/// @param p Point to measure.
/// @return The signed distance.
float Plane::signedDistance(const Vec3& p) const noexcept { return normal.dot(p) + d; }
/// Returns the absolute distance from point p to the plane.
/// @param p Point to measure.
/// @return The absolute distance.
float Plane::distance(const Vec3& p) const noexcept { return std::abs(signedDistance(p)); }

/// True when point p lies inside rect r (exclusive of right/bottom edges).
/// @param p Point to test.
/// @param r Rect.
/// @return True when inside.
bool pointInRect(const Point2& p, const RectI& r) noexcept {
    return p.x >= r.x && p.y >= r.y && p.x < r.right() && p.y < r.bottom();
}

/// True when rects a and b overlap.
/// @param a First rect.
/// @param b Second rect.
/// @return True when overlapping.
bool rectsOverlap(const RectI& a, const RectI& b) noexcept {
    return a.x < b.right() && a.right() > b.x && a.y < b.bottom() && a.bottom() > b.y;
}

/// True when point p lies within radius+eps of circle c.
/// @param c Circle.
/// @param p Point to test.
/// @param eps Extra tolerance.
/// @return True when contained.
bool circleContainsPoint(const Circle& c, const Vec2& p, float eps) noexcept {
    return (p - c.center).squaredLength() <= (c.radius + eps) * (c.radius + eps);
}

/// True when circles a and b overlap.
/// @param a First circle.
/// @param b Second circle.
/// @return True when overlapping.
bool circlesOverlap(const Circle& a, const Circle& b) noexcept {
    float rr = a.radius + b.radius;
    return (a.center - b.center).squaredLength() <= rr * rr;
}

/// True when rect r lies entirely inside circle c.
/// @param c Circle.
/// @param r Rect.
/// @return True when contained.
bool circleContainsRect(const Circle& c, const RectF& r) noexcept {
    float cx = std::abs(c.center.x - clamp(c.center.x, r.left(), r.right()));
    float cy = std::abs(c.center.y - clamp(c.center.y, r.top(), r.bottom()));
    return cx * cx + cy * cy <= c.radius * c.radius;
}

/// True when circle c and rect r intersect.
/// @param c Circle.
/// @param r Rect.
/// @return True when overlapping.
bool circleOverlapsRect(const Circle& c, const RectF& r) noexcept {
    float cx = clamp(c.center.x, r.left(), r.right()) - c.center.x;
    float cy = clamp(c.center.y, r.top(), r.bottom()) - c.center.y;
    return cx * cx + cy * cy <= c.radius * c.radius;
}

/// True when point p lies inside or on sphere s.
/// @param s Sphere.
/// @param p Point to test.
/// @return True when contained.
bool sphereContainsPoint(const Sphere& s, const Vec3& p) noexcept {
    return (p - s.center).squaredLength() <= s.radius * s.radius;
}

/// Casts ray against sphere s; reports the nearest hit distance.
/// @param ray Ray to cast.
/// @param s Sphere.
/// @param tOut Nearest hit distance (set on success).
/// @return True when the ray hits.
bool rayIntersectsSphere(const Ray& ray, const Sphere& s, float& tOut) noexcept {
    Vec3 oc = ray.origin - s.center;
    float b = oc.dot(ray.direction);
    float c = oc.dot(oc) - s.radius * s.radius;
    float disc = b * b - c;
    if (disc < 0)
        return false;
    float sq = std::sqrt(disc);
    float t0 = -b - sq;
    float t1 = -b + sq;
    tOut = t0 > 0 ? t0 : t1;
    return tOut >= 0;
}

/// Casts ray against plane; reports the hit distance.
/// @param ray Ray to cast.
/// @param plane Plane.
/// @param tOut Hit distance (set on success).
/// @return True when the ray hits (not parallel).
bool rayIntersectsPlane(const Ray& ray, const Plane& plane, float& tOut) noexcept {
    float denom = plane.normal.dot(ray.direction);
    if (std::abs(denom) < 1e-8f)
        return false;
    tOut = -(plane.normal.dot(ray.origin) + plane.d) / denom;
    return tOut >= 0;
}

/// Casts ray against an axis-aligned box, reporting entered/exited distances.
/// @param ray Ray to cast.
/// @param box Box.
/// @param tMin Entry distance (set on success).
/// @param tMax Exit distance (set on success).
/// @return True when the ray hits.
bool rayIntersectsAABB(const Ray& ray, const AABB3& box, float& tMin, float& tMax) noexcept {
    float tmin = 0, tmax = 1e30f;
    const Vec3& inv = {1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z};
    for (int axis = 0; axis < 3; ++axis) {
        float o = axis == 0 ? ray.origin.x : (axis == 1 ? ray.origin.y : ray.origin.z);
        float id = axis == 0 ? inv.x : (axis == 1 ? inv.y : inv.z);
        float lo = axis == 0 ? box.min.x : (axis == 1 ? box.min.y : box.min.z);
        float hi = axis == 0 ? box.max.x : (axis == 1 ? box.max.y : box.max.z);
        float t1 = (lo - o) * id;
        float t2 = (hi - o) * id;
        if (t1 > t2) {
            float tmp = t1;
            t1 = t2;
            t2 = tmp;
        }
        tmin = maxVal(tmin, t1);
        tmax = minVal(tmax, t2);
        if (tmin > tmax)
            return false;
    }
    tMin = tmin;
    tMax = tmax;
    return true;
}

/// Returns the closest point to p on the infinite line.
/// @param p Query point.
/// @param line Line.
/// @return The closest point.
Vec2 closestPointOnLine(const Vec2& p, const Line2& line) noexcept {
    Vec2 ab = line.b - line.a;
    float s2 = ab.squaredLength();
    if (s2 <= 1e-12f)
        return line.a;
    float t = (p - line.a).dot(ab) / s2;
    return line.a + ab * t;
}

/// Returns the closest point to p on the segment.
/// @param p Query point.
/// @param seg Segment.
/// @return The closest point.
Vec2 closestPointOnSegment(const Vec2& p, const Segment2& seg) noexcept {
    Vec2 ab = seg.b - seg.a;
    float s2 = ab.squaredLength();
    if (s2 <= 1e-12f)
        return seg.a;
    float t = clamp((p - seg.a).dot(ab) / s2, 0.0f, 1.0f);
    return seg.a + ab * t;
}

/// Returns the distance from p to the segment.
/// @param p Query point.
/// @param seg Segment.
/// @return The distance.
float distanceToSegment(const Vec2& p, const Segment2& seg) noexcept {
    return (p - closestPointOnSegment(p, seg)).length();
}

/// Returns the distance from p to the infinite line.
/// @param p Query point.
/// @param line Line.
/// @return The distance.
float distanceToLine(const Vec2& p, const Line2& line) noexcept {
    return std::abs(cross(line.b - line.a, p - line.a)) / (line.b - line.a).length();
}

/// Returns the closest point to p on the plane.
/// @param p Query point.
/// @param plane Plane.
/// @return The closest point.
Vec3 closestPointOnPlane(const Vec3& p, const Plane& plane) noexcept {
    return p - plane.normal * plane.signedDistance(p);
}

/// Returns the distance from p to the plane.
/// @param p Query point.
/// @param plane Plane.
/// @return The absolute distance.
float distanceToPlane(const Vec3& p, const Plane& plane) noexcept { return plane.distance(p); }

/// Returns the closest point to p on the segment from a to b.
/// @param p Query point.
/// @param a Segment start.
/// @param b Segment end.
/// @return The closest point.
Vec3 closestPointOnSegment3(const Vec3& p, const Vec3& a, const Vec3& b) noexcept {
    Vec3 ab = b - a;
    float s2 = ab.squaredLength();
    if (s2 <= 1e-12f)
        return a;
    float t = clamp((p - a).dot(ab) / s2, 0.0f, 1.0f);
    return a + ab * t;
}

/// Returns the closest point to p on the triangle.
/// @param p Query point.
/// @param tri Triangle.
/// @return The closest point.
Vec3 closestPointOnTriangle(const Vec3& p, const Triangle3& tri) noexcept {
    const Vec3& a = tri.a;
    const Vec3& b = tri.b;
    const Vec3& c = tri.c;
    Vec3 ab = b - a, ac = c - a, ap = p - a;
    float d1 = ab.dot(ap), d2 = ac.dot(ap);
    if (d1 <= 0 && d2 <= 0)
        return a;

    Vec3 bp = p - b;
    float d3 = ab.dot(bp), d4 = ac.dot(bp);
    if (d3 >= 0 && d4 <= d3)
        return b;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        float v = d1 / (d1 - d3);
        return a + ab * v;
    }

    Vec3 cp = p - c;
    float d5 = ab.dot(cp), d6 = ac.dot(cp);
    if (d6 >= 0 && d5 <= d6)
        return c;

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        float w = d2 / (d2 - d6);
        return a + ac * w;
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + (c - b) * w;
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return a + ab * v + ac * w;
}

/// Returns the area of a 2D triangle.
/// @param t Triangle.
/// @return The area.
float triangleArea2D(const Triangle2& t) noexcept {
    return std::abs(cross(t.b - t.a, t.c - t.a)) * 0.5f;
}

/// Returns the centroid of a 2D triangle.
/// @param t Triangle.
/// @return The centroid.
Vec2 triangleCentroid2D(const Triangle2& t) noexcept { return (t.a + t.b + t.c) / 3.0f; }

/// Returns the barycentric coordinates of p with respect to tri.
/// @param p Query point.
/// @param tri Triangle.
/// @return Coordinates (u, v, w) summing to 1.
Vec3 barycentric(const Vec2& p, const Triangle2& tri) noexcept {
    Vec2 v0 = tri.b - tri.a;
    Vec2 v1 = tri.c - tri.a;
    Vec2 v2 = p - tri.a;
    float d00 = v0.dot(v0), d01 = v0.dot(v1), d11 = v1.dot(v1);
    float d20 = v2.dot(v0), d21 = v2.dot(v1);
    float denom = d00 * d11 - d01 * d01;
    if (std::abs(denom) < 1e-12f)
        return Vec3(1, 0, 0);
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    return Vec3(1 - v - w, v, w);
}

/// True when point p lies inside triangle tri.
/// @param p Point to test.
/// @param tri Triangle.
/// @return True when inside.
bool triangleContains2D(const Vec2& p, const Triangle2& tri) noexcept {
    Vec3 b = barycentric(p, tri);
    return b.x >= 0 && b.y >= 0 && b.z >= 0;
}

/// Clips segment a-b against rect r (Liang-Barsky).
/// @param a First endpoint.
/// @param b Second endpoint.
/// @param r Clipping rect.
/// @param outA Clipped first endpoint (set on success).
/// @param outB Clipped second endpoint (set on success).
/// @return False when fully outside.
bool clipSegment(const Vec2& a, const Vec2& b, const RectF& r, Vec2& outA, Vec2& outB) noexcept {
    float t0 = 0.0f, t1 = 1.0f;
    Vec2 d = b - a;
    const float p[4] = {-d.x, d.x, -d.y, d.y};
    const float q[4] = {a.x - r.left(), r.right() - a.x, a.y - r.top(), r.bottom() - a.y};
    for (int i = 0; i < 4; ++i) {
        if (std::abs(p[i]) < 1e-12f) {
            if (q[i] < 0)
                return false;
        } else {
            float t = q[i] / p[i];
            if (p[i] < 0) {
                if (t > t1)
                    return false;
                if (t > t0)
                    t0 = t;
            } else {
                if (t < t0)
                    return false;
                if (t < t1)
                    t1 = t;
            }
        }
    }
    outA = a + d * t0;
    outB = a + d * t1;
    return true;
}

/// Clips a polygon against rect r (Sutherland-Hodgman).
/// @param in Input polygon vertices.
/// @param r Clipping rect.
/// @return The clipped polygon (may be empty).
std::vector<Vec2> clipPolygonToRect(const std::vector<Vec2>& in, const RectF& r) {
    std::vector<Vec2> out = in;
    auto clipEdge = [&](bool left) {
        std::vector<Vec2> next;
        next.reserve(out.size() + 2);
        for (size_t i = 0; i < out.size(); ++i) {
            const Vec2& cur = out[i];
            const Vec2& prev = out[(i + out.size() - 1) % out.size()];
            auto inside = [&](const Vec2& p) { return left ? p.x >= r.left() : p.y >= r.top(); };
            bool curIn = inside(cur), prevIn = inside(prev);
            if (curIn) {
                if (!prevIn) {
                    float t = left ? (r.left() - prev.x) / (cur.x - prev.x)
                                   : (r.top() - prev.y) / (cur.y - prev.y);
                    next.push_back(prev + (cur - prev) * t);
                }
                next.push_back(cur);
            } else if (prevIn) {
                float t = left ? (r.left() - prev.x) / (cur.x - prev.x)
                               : (r.top() - prev.y) / (cur.y - prev.y);
                next.push_back(prev + (cur - prev) * t);
            }
        }
        out.swap(next);
    };
    clipEdge(true);
    if (out.empty())
        return out;
    clipEdge(false);
    if (out.empty())
        return out;
    std::vector<Vec2> next;
    next.reserve(out.size() + 2);
    for (size_t i = 0; i < out.size(); ++i) {
        const Vec2& cur = out[i];
        const Vec2& prev = out[(i + out.size() - 1) % out.size()];
        bool curIn = cur.x <= r.right(), prevIn = prev.x <= r.right();
        if (curIn) {
            if (!prevIn) {
                float t = (r.right() - prev.x) / (cur.x - prev.x);
                next.push_back(prev + (cur - prev) * t);
            }
            next.push_back(cur);
        } else if (prevIn) {
            float t = (r.right() - prev.x) / (cur.x - prev.x);
            next.push_back(prev + (cur - prev) * t);
        }
    }
    out.swap(next);
    if (out.empty())
        return out;
    next.clear();
    for (size_t i = 0; i < out.size(); ++i) {
        const Vec2& cur = out[i];
        const Vec2& prev = out[(i + out.size() - 1) % out.size()];
        bool curIn = cur.y <= r.bottom(), prevIn = prev.y <= r.bottom();
        if (curIn) {
            if (!prevIn) {
                float t = (r.bottom() - prev.y) / (cur.y - prev.y);
                next.push_back(prev + (cur - prev) * t);
            }
            next.push_back(cur);
        } else if (prevIn) {
            float t = (r.bottom() - prev.y) / (cur.y - prev.y);
            next.push_back(prev + (cur - prev) * t);
        }
    }
    out.swap(next);
    return out;
}

} // namespace math
} // namespace iml