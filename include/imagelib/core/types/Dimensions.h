#pragma once
#ifndef IMAGELIB_CORE_DIMENSIONS_H_
#define IMAGELIB_CORE_DIMENSIONS_H_
/// @file
/// 2D integer dimension, point, and rectangle types plus a 3D extent.

#include "imagelib/core/types/FixedTypes.h"

namespace iml {

/// Non-negative 2D size used for image dimensions.
struct Size {
    /// Width in pixels.
    uint32 width = 0;
    /// Height in pixels.
    uint32 height = 0;

    /// Default-constructs a size with zero width and height.
    constexpr Size() noexcept = default;
    /// Constructs a size from explicit width and height.
    /// @param w Width in pixels.
    /// @param h Height in pixels.
    constexpr Size(uint32 w, uint32 h) noexcept : width(w), height(h) {}

    /// Returns whether either dimension is zero.
    /// @return True when width or height is 0.
    constexpr bool empty() const noexcept { return width == 0 || height == 0; }
    /// Returns the total number of pixels.
    /// @return The product of width and height.
    constexpr uint32 pixels() const noexcept { return width * height; }

    /// Compares two sizes for equality.
    /// @param o The other size.
    /// @return True when both dimensions match.
    constexpr bool operator==(const Size& o) const noexcept {
        return width == o.width && height == o.height;
    }
    /// Compares two sizes for inequality.
    /// @param o The other size.
    /// @return True when either dimension differs.
    constexpr bool operator!=(const Size& o) const noexcept { return !(*this == o); }
};

/// Signed 2D integer point.
struct Point {
    /// X coordinate.
    int32 x = 0;
    /// Y coordinate.
    int32 y = 0;

    /// Default-constructs a point at the origin.
    constexpr Point() noexcept = default;
    /// Constructs a point from explicit coordinates.
    /// @param px X coordinate.
    /// @param py Y coordinate.
    constexpr Point(int32 px, int32 py) noexcept : x(px), y(py) {}

    /// Adds another point.
    /// @param o The point to add.
    /// @return The sum of the two points.
    constexpr Point operator+(const Point& o) const noexcept { return {x + o.x, y + o.y}; }
    /// Subtracts another point.
    /// @param o The point to subtract.
    /// @return The difference of the two points.
    constexpr Point operator-(const Point& o) const noexcept { return {x - o.x, y - o.y}; }
    /// Adds another point in place.
    /// @param o The point to add.
    /// @return Reference to this point.
    Point& operator+=(const Point& o) noexcept {
        x += o.x;
        y += o.y;
        return *this;
    }
    /// Subtracts another point in place.
    /// @param o The point to subtract.
    /// @return Reference to this point.
    Point& operator-=(const Point& o) noexcept {
        x -= o.x;
        y -= o.y;
        return *this;
    }

    /// Compares two points for equality.
    /// @param o The other point.
    /// @return True when both coordinates match.
    constexpr bool operator==(const Point& o) const noexcept { return x == o.x && y == o.y; }
    /// Compares two points for inequality.
    /// @param o The other point.
    /// @return True when either coordinate differs.
    constexpr bool operator!=(const Point& o) const noexcept { return !(*this == o); }
};

/// Signed 2D rectangle: origin at (x, y) with extent (width, height).
struct Rect {
    /// Left coordinate of the origin.
    int32 x = 0;
    /// Top coordinate of the origin.
    int32 y = 0;
    /// Width in pixels.
    uint32 width = 0;
    /// Height in pixels.
    uint32 height = 0;

    /// Default-constructs an empty rectangle at the origin.
    constexpr Rect() noexcept = default;
    /// Constructs a rectangle from an origin and extent.
    /// @param px X coordinate of the origin.
    /// @param py Y coordinate of the origin.
    /// @param w Width in pixels.
    /// @param h Height in pixels.
    constexpr Rect(int32 px, int32 py, uint32 w, uint32 h) noexcept
        : x(px), y(py), width(w), height(h) {}

    /// Returns whether either extent dimension is zero.
    /// @return True when the rectangle has no area.
    constexpr bool empty() const noexcept { return width == 0 || height == 0; }

    /// Bounds in a signed coordinate system.
    /// Left edge X coordinate.
    /// @return The value of x.
    constexpr int32 left() const noexcept { return x; }
    /// Top edge Y coordinate.
    /// @return The value of y.
    constexpr int32 top() const noexcept { return y; }
    /// Exclusive right edge X coordinate.
    /// @return x plus width.
    constexpr int32 right() const noexcept { return x + static_cast<int32>(width); }
    /// Exclusive bottom edge Y coordinate.
    /// @return y plus height.
    constexpr int32 bottom() const noexcept { return y + static_cast<int32>(height); }

    /// Returns whether the point lies inside this rectangle.
    /// @param p The point to test.
    /// @return True when p is within [x, x+width) x [y, y+height).
    constexpr bool contains(const Point& p) const noexcept {
        return p.x >= x && p.x < right() && p.y >= y && p.y < bottom();
    }

    /// Intersection of two rects (assumes this is within a [0, W) x [0, H) image).
    /// @param o The other rectangle.
    /// @return The overlap, or an empty rect when disjoint.
    Rect intersect(const Rect& o) const noexcept {
        int32 l = x > o.x ? x : o.x;
        int32 t = y > o.y ? y : o.y;
        int32 r = right() < o.right() ? right() : o.right();
        int32 b = bottom() < o.bottom() ? bottom() : o.bottom();
        if (r <= l || b <= t)
            return Rect();
        return Rect(l, t, static_cast<uint32>(r - l), static_cast<uint32>(b - t));
    }

    /// Compares two rectangles for equality.
    /// @param o The other rectangle.
    /// @return True when origin and extent match.
    constexpr bool operator==(const Rect& o) const noexcept {
        return x == o.x && y == o.y && width == o.width && height == o.height;
    }
    /// Compares two rectangles for inequality.
    /// @param o The other rectangle.
    /// @return True when the rectangles differ.
    constexpr bool operator!=(const Rect& o) const noexcept { return !(*this == o); }
};

/// 3D size / offset used by optional volumetric data.
struct Extent {
    /// Width in elements.
    uint32 width = 0;
    /// Height in elements.
    uint32 height = 0;
    /// Depth in elements.
    uint32 depth = 1;

    /// Default-constructs an extent with zero area and unit depth.
    constexpr Extent() noexcept = default;
    /// Constructs an extent from dimensions, defaulting depth to 1.
    /// @param w Width in elements.
    /// @param h Height in elements.
    /// @param d Depth in elements.
    constexpr Extent(uint32 w, uint32 h, uint32 d = 1) noexcept : width(w), height(h), depth(d) {}

    /// Returns whether any dimension is zero.
    /// @return True when width, height, or depth is 0.
    constexpr bool empty() const noexcept { return width == 0 || height == 0 || depth == 0; }
    /// Returns the total number of elements.
    /// @return The product of all three dimensions.
    constexpr uint64 elements() const noexcept {
        return static_cast<uint64>(width) * height * depth;
    }

    /// Compares two extents for equality.
    /// @param o The other extent.
    /// @return True when all dimensions match.
    constexpr bool operator==(const Extent& o) const noexcept {
        return width == o.width && height == o.height && depth == o.depth;
    }
    /// Compares two extents for inequality.
    /// @param o The other extent.
    /// @return True when any dimension differs.
    constexpr bool operator!=(const Extent& o) const noexcept { return !(*this == o); }
};

} // namespace iml

#endif