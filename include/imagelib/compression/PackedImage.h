#pragma once
// imagelib/compression/PackedImage.h
//
// Owning container for packed pixel data (sub-byte or packed-16 formats) and
// its optional RGBA palette.
//
// Ownership: PackedImage owns its pixel memory and palette through mem::Buffer
// (RAII, deep-copy copy semantics, transfer on move). No smart pointers.
//
// Thread safety: distinct instances are independent; a single instance must
// not be mutated concurrently.

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/core/Image.h"
#include "imagelib/core/memory/Buffer.h"
#include "imagelib/compression/PackedPixels.h"

#include <utility>

namespace iml {
namespace comp {

class PackedImage {
public:
    static constexpr uint32 defaultRowAlignment = 4;

    PackedImage() noexcept = default;

    /// Reserves a packed canvas (uninitialized pixel storage).
    PackedImage(PackedFormat fmt, uint32 w, uint32 h,
                ColorSpace cs = ColorSpace::Unknown,
                uint32 rowAlignment = defaultRowAlignment);

    /// Packs `src` immediately (canonical RGBA ordering depends on src's
    /// PixelFormat; index lookup uses the given palette when fmt is indexed).
    explicit PackedImage(const Image& src, PackedFormat fmt,
                         const byte* palette = nullptr, size_t paletteCount = 0,
                         uint32 rowAlignment = defaultRowAlignment);

    PackedImage(const PackedImage& other);
    PackedImage& operator=(const PackedImage& other);
    PackedImage(PackedImage&& other) noexcept { swap(other); }
    PackedImage& operator=(PackedImage&& other) noexcept {
        if (this != &other) { release(); swap(other); }
        return *this;
    }
    ~PackedImage() = default;

    void swap(PackedImage& other) noexcept;
    void release() noexcept;

    PackedImage clone() const;

    // -- layout -------------------------------------------------------------

    PackedFormat format() const noexcept { return format_; }
    uint32       bitDepth() const noexcept { return bitsPerPixel(format_); }
    uint32       rowBytes() const noexcept { return rowBytes_; }
    uint32       width() const noexcept { return width_; }
    uint32       height() const noexcept { return height_; }
    ColorSpace   colorSpace() const noexcept { return colorSpace_; }
    uint32       rowAlignment() const noexcept { return rowAlign_; }
    bool         indexed() const noexcept { return isIndexed(format_); }
    bool         empty() const noexcept { return width_ == 0 || height_ == 0; }
    uint64       pixelCount() const noexcept { return static_cast<uint64>(width_) * height_; }

    // -- palette ------------------------------------------------------------

    const byte* palette() const noexcept { return palette_.data(); }
    size_t      paletteCount() const noexcept { return paletteCount_; }
    void        setPalette(const byte* rgba, size_t count);

    // -- access -------------------------------------------------------------

    byte* data() noexcept { return pixels_.data(); }
    const byte* data() const noexcept { return pixels_.data(); }

    uint64 readPixel(uint32 x, uint32 y) const noexcept {
        return readPackedPixel(pixels_.data() + static_cast<uint64>(y) * rowBytes_,
                               x, format_);
    }
    void writePixel(uint32 x, uint32 y, uint64 value) noexcept {
        writePackedPixel(pixels_.data() + static_cast<uint64>(y) * rowBytes_,
                         x, format_, value);
    }

    // -- conversions --------------------------------------------------------

    /// Packs this image from a UInt8 source; replaces contents.
    void pack(const Image& src, const byte* palette, size_t paletteCount);

    /// Decodes into a new UInt8 Image (RGBA for color formats, indexed formats
    /// expand through the palette).
    Image unpack(ColorSpace cs = ColorSpace::Unknown) const;

    void unpackTo(Image& out, ColorSpace csOverride = ColorSpace::Unknown) const;

private:
    void allocate(PackedFormat fmt, uint32 w, uint32 h, ColorSpace cs,
                  uint32 rowAlignment);

    PackedFormat format_ = PackedFormat::Gray8;
    uint32       width_ = 0;
    uint32       height_ = 0;
    uint32       rowBytes_ = 0;
    uint32       rowAlign_ = defaultRowAlignment;
    ColorSpace   colorSpace_ = ColorSpace::Unknown;
    mem::Buffer  pixels_;
    mem::Buffer  palette_;        // RGBA, paletteCount_ entries
    size_t       paletteCount_ = 0;
};

inline void swap(PackedImage& a, PackedImage& b) noexcept { a.swap(b); }

} // namespace comp
} // namespace iml