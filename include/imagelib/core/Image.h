#pragma once
#define IMAGELIB_CORE_IMAGE_H_
// imagelib/core/Image.h
//
// The primary owning image container.
//
// Ownership: Image owns its pixel memory through mem::Buffer; it is the single
// owner. Copying performs a deep copy, moving transfers ownership, and the
// destructor releases memory. No smart pointers.
//
// Thread safety: distinct instances are independent; a single instance must
// not be mutated concurrently.
//
// load()/save() are routed through the codec registry (see codecs/Codec.h)
// but Image itself has no knowledge of any concrete codec.

#include "imagelib/core/Types.h"
#include "imagelib/core/ImageView.h"
#include "imagelib/core/memory/Buffer.h"
#include "imagelib/core/memory/Memory.h"
#include "imagelib/core/Error.h"

#include <string>
#include <utility>

namespace iml {

/// Options controlling integration when saving through a codec.
struct SaveOptions {
    int   jpegQuality       = 90;  ///< JPEG quality 0-100 (lossy).
    uint16 pngCompressionLevel = 6; ///< PNG zlib level 0-9.
    bool  preserveAlpha     = true;
};

class Image {
public:
    static constexpr uint32 defaultRowAlignment = 16; // SIMD-row friendly

    // -- construction ------------------------------------------------------

    Image() noexcept = default;

    explicit Image(const std::string& path);

    /// Allocates an empty (uninitialized) w x h image.
    Image(ImageFormat f, uint32 w, uint32 h,
          ColorSpace cs = ColorSpace::SRGB,
          AlphaMode am = AlphaMode::Unknown,
          uint32 rowAlignment = defaultRowAlignment);

    Image(const Image& other);
    Image& operator=(const Image& other);
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;
    ~Image() = default;

    // -- load / save (codec layer) -----------------------------------------

    void load(const std::string& path);
    void save(const std::string& path, const SaveOptions& opts = {}) const;

    /// Deep copy returned by value.
    Image clone() const;

    /// Releases pixel memory; instance becomes empty and still valid.
    void release() noexcept;

    void swap(Image& other) noexcept;

    // -- metadata ----------------------------------------------------------

    bool    empty() const noexcept;
    uint32  width()  const noexcept;
    uint32  height() const noexcept;
    uint16  channels() const noexcept;
    uint32  stride() const noexcept;
    Size    size() const noexcept;
    const ImageFormat& format() const noexcept;
    PixelFormat pixelFormat() const noexcept;
    DataType    dataType() const noexcept;
    ColorSpace  colorSpace() const noexcept;
    AlphaMode   alphaMode() const noexcept;
    uint32  pixelSizeBytes() const noexcept;
    uint32  bytesPerChannel() const noexcept;
    uint32  rowAlign() const noexcept;

    bool hasAlpha() const noexcept;

    /// Byte size of the backing buffer (stride * height).
    uint64 bufferBytes() const noexcept;

    // -- raw access --------------------------------------------------------

    byte* data() noexcept;
    const byte* data() const noexcept;
    byte* row(int32 y) noexcept;
    const byte* row(int32 y) const noexcept;
    byte* pixel(int32 x, int32 y) noexcept;
    const byte* pixel(int32 x, int32 y) const noexcept;
    byte* sample(int32 x, int32 y, uint32 c) noexcept;
    const byte* sample(int32 x, int32 y, uint32 c) const noexcept;

    // -- typed access ------------------------------------------------------

    /// Reads sample as T.
    template <typename T>
    T get(int32 x, int32 y, uint32 c) const;

    /// Writes a sample whose store width matches the image data type.
    template <typename T>
    void set(int32 x, int32 y, uint32 c, T value);

    /// Convenience byte accessor for 8-bit images (legacy-friendly).
    byte& operator()(int32 x, int32 y, uint32 c);
    byte operator()(int32 x, int32 y, uint32 c) const;

    // -- views -------------------------------------------------------------

    ImageView view() noexcept;
    ConstImageView view() const noexcept;
    ConstImageView cview() const noexcept;

    /// Crops in-place view semantics: returns an ImageView over this image.
    ImageView subView(uint32 x0, uint32 y0, uint32 w, uint32 h) noexcept;
    ConstImageView subView(uint32 x0, uint32 y0, uint32 w, uint32 h) const noexcept;

private:
    void allocate(ImageFormat f, uint32 w, uint32 h,
                  ColorSpace cs, AlphaMode am, uint32 rowAlignment);

    mem::Buffer buffer_;
    uint32      width_      = 0;
    uint32      height_     = 0;
    uint16      channels_   = 0;
    uint32      stride_     = 0;
    ImageFormat format_;
    ColorSpace  colorSpace_ = ColorSpace::Unknown;
    AlphaMode   alphaMode_  = AlphaMode::Unknown;
    uint32      rowAlign_   = defaultRowAlignment;
};

void swap(Image& a, Image& b) noexcept;

} // namespace iml

#include "imagelib/core/Image.tpp"