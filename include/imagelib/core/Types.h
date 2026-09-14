#pragma once
// imagelib/core/Types.h
//
// Core fixed-width types and shared descriptors used across ImageLib.
//
// Ownership: none (all value types).
// Thread safety: values are immutable once constructed; trivially copyable.

#include <cstdint>
#include <cstddef>

namespace iml {

// ---------------------------------------------------------------------------
// Fixed-width type aliases.
// ---------------------------------------------------------------------------
using int8   = std::int8_t;
using uint8  = std::uint8_t;
using int16  = std::int16_t;
using uint16 = std::uint16_t;
using int32  = std::int32_t;
using uint32 = std::uint32_t;
using int64  = std::int64_t;
using uint64 = std::uint64_t;

using byte = uint8;

// ---------------------------------------------------------------------------
// Image data type.
// ---------------------------------------------------------------------------
enum class DataType : uint8 {
    // nullptr marker == "no image" / unset
    Unknown = 0,
    UInt8,
    UInt16,
    UInt32,
    Float16,
    Float32,
};

constexpr const char* toString(DataType t) noexcept {
    switch (t) {
        case DataType::Unknown: return "Unknown";
        case DataType::UInt8:   return "UInt8";
        case DataType::UInt16:  return "UInt16";
        case DataType::UInt32:  return "UInt32";
        case DataType::Float16: return "Float16";
        case DataType::Float32: return "Float32";
    }
    return "Unknown";
}

/// Size in bytes of one scalar sample (+1) for a data type. 0 when unsupported
/// as a storage type.
constexpr size_t dataTypeSize(DataType t) noexcept {
    switch (t) {
        case DataType::UInt8:   return sizeof(uint8);
        case DataType::UInt16:  return sizeof(uint16);
        case DataType::UInt32:  return sizeof(uint32);
        case DataType::Float32: return sizeof(float);
        case DataType::Float16: return sizeof(uint16); // storage width of f16
        case DataType::Unknown: return 0;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Pixel format: the layout of channels within one pixel.
// ---------------------------------------------------------------------------
enum class PixelFormat : uint8 {
    Unknown = 0,
    Gray,        // 1 channel
    GrayAlpha,   // 2 channels
    RGB,         // 3 channels
    BGR,         // 3 channels, reversed
    RGBA,        // 4 channels
    BGRA,        // 4 channels, reversed
    Packed,      // arbitrary packed bit layout (see compression/PackedPixels.h)
};

constexpr uint32 pixelFormatChannels(PixelFormat f) noexcept {
    switch (f) {
        case PixelFormat::Gray:       return 1;
        case PixelFormat::GrayAlpha:  return 2;
        case PixelFormat::RGB:
        case PixelFormat::BGR:        return 3;
        case PixelFormat::RGBA:
        case PixelFormat::BGRA:       return 4;
        case PixelFormat::Unknown:    return 0;
        case PixelFormat::Packed:     return 0;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Image format: a pixel format + data type pairing.
// ---------------------------------------------------------------------------
struct ImageFormat {
    PixelFormat pixelFormat = PixelFormat::Unknown;
    DataType    dataType    = DataType::Unknown;

    constexpr ImageFormat() noexcept = default;
    constexpr ImageFormat(PixelFormat pf, DataType dt) noexcept
        : pixelFormat(pf), dataType(dt) {}

    constexpr bool valid() const noexcept {
        return pixelFormat != PixelFormat::Unknown && dataType != DataType::Unknown;
    }
    constexpr bool operator==(const ImageFormat& o) const noexcept {
        return pixelFormat == o.pixelFormat && dataType == o.dataType;
    }
    constexpr bool operator!=(const ImageFormat& o) const noexcept {
        return !(*this == o);
    }
    /// Number of channels implied by the pixel format.
    constexpr uint32 channels() const noexcept { return pixelFormatChannels(pixelFormat); }
    /// Bytes per sample.
    constexpr size_t sampleSize() const noexcept { return dataTypeSize(dataType); }
    /// Bytes per pixel (bytes-per-channel * channels).
    constexpr size_t pixelSize() const noexcept { return sampleSize() * channels(); }
};

/// Common image format presets.
namespace fmt {
constexpr ImageFormat gray8   = ImageFormat(PixelFormat::Gray, DataType::UInt8);
constexpr ImageFormat gray16  = ImageFormat(PixelFormat::Gray, DataType::UInt16);
constexpr ImageFormat gray32f = ImageFormat(PixelFormat::Gray, DataType::Float32);
constexpr ImageFormat rgb8    = ImageFormat(PixelFormat::RGB,  DataType::UInt8);
constexpr ImageFormat rgba8   = ImageFormat(PixelFormat::RGBA, DataType::UInt8);
constexpr ImageFormat bgr8    = ImageFormat(PixelFormat::BGR,  DataType::UInt8);
constexpr ImageFormat bgra8   = ImageFormat(PixelFormat::BGRA, DataType::UInt8);
constexpr ImageFormat rgb32f  = ImageFormat(PixelFormat::RGB,  DataType::Float32);
constexpr ImageFormat rgba32f = ImageFormat(PixelFormat::RGBA, DataType::Float32);
} // namespace fmt

// ---------------------------------------------------------------------------
// Color space.
// ---------------------------------------------------------------------------
enum class ColorSpace : uint8 {
    Unknown = 0,
    Linear,       // linear RGBA light
    SRGB,         // sRGB encoded
    Gray,         // luminance-only
    HSV,
    HSL,
    XYZ,
    LAB,
    YCbCr,
};

constexpr const char* toString(ColorSpace c) noexcept {
    switch (c) {
        case ColorSpace::Unknown: return "Unknown";
        case ColorSpace::Linear:  return "Linear";
        case ColorSpace::SRGB:    return "sRGB";
        case ColorSpace::Gray:    return "Gray";
        case ColorSpace::HSV:     return "HSV";
        case ColorSpace::HSL:     return "HSL";
        case ColorSpace::XYZ:     return "XYZ";
        case ColorSpace::LAB:     return "LAB";
        case ColorSpace::YCbCr:   return "YCbCr";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// Alpha mode.
// ---------------------------------------------------------------------------
enum class AlphaMode : uint8 {
    Unknown = 0,
    None,          // no alpha channel present
    Straight,      // color not multiplied by alpha
    Premultiplied, // color already multiplied by alpha
};

// ---------------------------------------------------------------------------
// 2D integer dimension / point / rectangle types.
// ---------------------------------------------------------------------------

/// Non-negative 2D size used for image dimensions.
struct Size {
    uint32 width  = 0;
    uint32 height = 0;

    constexpr Size() noexcept = default;
    constexpr Size(uint32 w, uint32 h) noexcept : width(w), height(h) {}

    constexpr bool empty() const noexcept { return width == 0 || height == 0; }
    constexpr uint32 pixels() const noexcept { return width * height; }

    constexpr bool operator==(const Size& o) const noexcept {
        return width == o.width && height == o.height;
    }
    constexpr bool operator!=(const Size& o) const noexcept { return !(*this == o); }
};

/// Signed 2D integer point.
struct Point {
    int32 x = 0;
    int32 y = 0;

    constexpr Point() noexcept = default;
    constexpr Point(int32 px, int32 py) noexcept : x(px), y(py) {}

    constexpr Point operator+(const Point& o) const noexcept { return {x + o.x, y + o.y}; }
    constexpr Point operator-(const Point& o) const noexcept { return {x - o.x, y - o.y}; }
    Point& operator+=(const Point& o) noexcept { x += o.x; y += o.y; return *this; }
    Point& operator-=(const Point& o) noexcept { x -= o.x; y -= o.y; return *this; }

    constexpr bool operator==(const Point& o) const noexcept { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Point& o) const noexcept { return !(*this == o); }
};

/// Signed 2D rectangle: origin at (x, y) with extent (width, height).
struct Rect {
    int32  x      = 0;
    int32  y      = 0;
    uint32 width  = 0;
    uint32 height = 0;

    constexpr Rect() noexcept = default;
    constexpr Rect(int32 px, int32 py, uint32 w, uint32 h) noexcept
        : x(px), y(py), width(w), height(h) {}

    constexpr bool empty() const noexcept { return width == 0 || height == 0; }

    /// Bounds in a signed coordinate system.
    constexpr int32 left()   const noexcept { return x; }
    constexpr int32 top()    const noexcept { return y; }
    constexpr int32 right()  const noexcept { return x + static_cast<int32>(width); }
    constexpr int32 bottom() const noexcept { return y + static_cast<int32>(height); }

    constexpr bool contains(const Point& p) const noexcept {
        return p.x >= x && p.x < right() && p.y >= y && p.y < bottom();
    }

    /// Intersection of two rects (assumes this is within a [0, W) x [0, H) image).
    Rect intersect(const Rect& o) const noexcept {
        int32  l = x > o.x ? x : o.x;
        int32  t = y > o.y ? y : o.y;
        int32  r = right()  < o.right()  ? right()  : o.right();
        int32  b = bottom() < o.bottom() ? bottom() : o.bottom();
        if (r <= l || b <= t) return Rect();
        return Rect(l, t, static_cast<uint32>(r - l), static_cast<uint32>(b - t));
    }

    constexpr bool operator==(const Rect& o) const noexcept {
        return x == o.x && y == o.y && width == o.width && height == o.height;
    }
    constexpr bool operator!=(const Rect& o) const noexcept { return !(*this == o); }
};

/// 3D size / offset used by optional volumetric data.
struct Extent {
    uint32 width  = 0;
    uint32 height = 0;
    uint32 depth  = 1;

    constexpr Extent() noexcept = default;
    constexpr Extent(uint32 w, uint32 h, uint32 d = 1) noexcept
        : width(w), height(h), depth(d) {}

    constexpr bool empty() const noexcept {
        return width == 0 || height == 0 || depth == 0;
    }
    constexpr uint64 elements() const noexcept {
        return static_cast<uint64>(width) * height * depth;
    }

    constexpr bool operator==(const Extent& o) const noexcept {
        return width == o.width && height == o.height && depth == o.depth;
    }
    constexpr bool operator!=(const Extent& o) const noexcept { return !(*this == o); }
};

} // namespace iml