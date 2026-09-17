#pragma once
#define IMAGELIB_CORE_IMAGE_H_

/// @file Image.h
/// The primary owning image container; owns pixel memory through mem::Buffer
/// (deep copy on copy, ownership transfer on move).

#include "imagelib/core/Types.h"
#include "imagelib/core/image/ImageView.h"
#include "imagelib/core/image/Pixel.h"
#include "imagelib/core/memory/Buffer.h"
#include "imagelib/core/memory/Memory.h"
#include "imagelib/core/Error.h"

#include <string>
#include <utility>

namespace iml {

/// Options controlling integration when saving through a codec.
struct SaveOptions {
    int jpegQuality = 90;           ///< JPEG quality 0-100 (lossy).
    uint16 pngCompressionLevel = 6; ///< PNG zlib level 0-9; level 0 uses fast stored blocks.
    bool preserveAlpha = true;      ///< Keeps the alpha channel when saving.
};

/// Image owning pixel memory via mem::Buffer; deep-copies on copy, moves
/// transfer ownership.
class Image {
  public:
    /// Default row alignment in bytes (SIMD friendly).
    static constexpr uint32 defaultRowAlignment = 16;

    /// Constructs an empty image.
    Image() noexcept = default;

    /// Loads an image from the given path.
    /// @param path The source image file.
    /// @throws Codec or format errors from the codec layer.
    explicit Image(const std::string& path);

    /// Allocates an empty (uninitialized) w x h image.
    /// @param f Pixel format of the image.
    /// @param w Width in pixels.
    /// @param h Height in pixels.
    /// @param cs Color space; defaults to sRGB.
    /// @param am Alpha mode; defaults to Unknown.
    /// @param rowAlignment Row alignment in bytes.
    /// @throws InvalidParameterError on an invalid format or alignment.
    /// @throws InvalidDimensionError on zero dimensions.
    /// @throws UnsupportedFormatError when more than 4 channels are requested.
    Image(
        ImageFormat f,
        uint32 w,
        uint32 h,
        ColorSpace cs = ColorSpace::SRGB,
        AlphaMode am = AlphaMode::Unknown,
        uint32 rowAlignment = defaultRowAlignment);
    /// Copies another image (deep copy of pixel memory).
    /// @param other The source image.
    Image(const Image& other);
    /// Deep-copy assignment.
    /// @param other The source image.
    /// @return Reference to this image.
    Image& operator=(const Image& other);
    /// Moves pixel memory from another image.
    /// @param other The source image.
    Image(Image&& other) noexcept;
    /// Move assignment, transferring ownership.
    /// @param other The source image.
    /// @return Reference to this image.
    Image& operator=(Image&& other) noexcept;
    /// Destroys the image and releases pixel memory.
    ~Image() = default;

    /// Loads an image from the given path through the codec registry.
    /// @param path The source image file.
    /// @throws Codec or format errors from the codec layer.
    void load(const std::string& path);
    /// Saves the image to the given path through the codec registry.
    /// @param path The destination image file.
    /// @param opts Save options.
    /// @throws Codec or format errors from the codec layer.
    void save(const std::string& path, const SaveOptions& opts = {}) const;

    /// Deep copy returned by value.
    /// @return A deep copy of this image.
    Image clone() const;

    /// Releases pixel memory; instance becomes empty and still valid.
    void release() noexcept;

    /// Swaps the contents of this image with another.
    /// @param other The image to swap with.
    void swap(Image& other) noexcept;

    /// Returns true when the image has no pixels (zero width or height).
    /// @return True when empty.
    bool empty() const noexcept;
    /// Returns the width in pixels.
    /// @return Width in pixels.
    uint32 width() const noexcept;
    /// Returns the height in pixels.
    /// @return Height in pixels.
    uint32 height() const noexcept;
    /// Returns the number of channels per pixel.
    /// @return Channel count.
    uint16 channels() const noexcept;
    /// Returns the row stride in bytes.
    /// @return Row stride in bytes.
    uint32 stride() const noexcept;
    /// Returns the width and height as a Size.
    /// @return Image size.
    Size size() const noexcept;
    /// Returns the pixel format and data type.
    /// @return The image format.
    const ImageFormat& format() const noexcept;
    /// Returns the pixel format.
    /// @return The pixel format.
    PixelFormat pixelFormat() const noexcept;
    /// Returns the sample data type.
    /// @return The data type.
    DataType dataType() const noexcept;
    /// Returns the color space.
    /// @return The color space.
    ColorSpace colorSpace() const noexcept;
    /// Returns the alpha mode.
    /// @return The alpha mode.
    AlphaMode alphaMode() const noexcept;
    /// Returns the number of bytes per pixel (channels * bytes per channel).
    /// @return Bytes per pixel.
    uint32 pixelSizeBytes() const noexcept;
    /// Returns the number of bytes per channel.
    /// @return Bytes per channel.
    uint32 bytesPerChannel() const noexcept;
    /// Returns the configured row alignment in bytes.
    /// @return Row alignment in bytes.
    uint32 rowAlign() const noexcept;

    /// Returns true when the layout stores an alpha channel.
    /// @return True when alpha is present.
    bool hasAlpha() const noexcept;

    /// Byte size of the backing buffer (stride * height).
    /// @return Backing buffer size in bytes.
    uint64 bufferBytes() const noexcept;

    /// Returns a pointer to the first pixel byte.
    /// @return Pointer to the backing buffer.
    byte* data() noexcept;
    /// Returns a const pointer to the first pixel byte.
    /// @return Const pointer to the backing buffer.
    const byte* data() const noexcept;
    /// Returns a pointer to the first byte of row y.
    /// @param y Row index.
    /// @return Pointer to the row.
    byte* row(int32 y) noexcept;
    /// Returns a const pointer to the first byte of row y.
    /// @param y Row index.
    /// @return Const pointer to the row.
    const byte* row(int32 y) const noexcept;
    /// Returns a pointer to the first byte of pixel (x, y).
    /// @param x Column index.
    /// @param y Row index.
    /// @return Pointer to the pixel.
    byte* pixel(int32 x, int32 y) noexcept;
    /// Returns a const pointer to the first byte of pixel (x, y).
    /// @param x Column index.
    /// @param y Row index.
    /// @return Const pointer to the pixel.
    const byte* pixel(int32 x, int32 y) const noexcept;
    /// Returns a pointer to sample channel c of pixel (x, y).
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @return Pointer to the sample.
    byte* sample(int32 x, int32 y, uint32 c) noexcept;
    /// Returns a const pointer to sample channel c of pixel (x, y).
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @return Const pointer to the sample.
    const byte* sample(int32 x, int32 y, uint32 c) const noexcept;

    /// Reads sample channel c of pixel (x, y) converted to T.
    /// @tparam T The sample type to read.
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @return The sample as T.
    /// @throws InvalidParameterError when out of bounds.
    /// @throws UnsupportedFormatError on unsupported data types.
    template <typename T> T get(int32 x, int32 y, uint32 c) const;

    /// Writes a sample whose store width matches the image data type.
    /// @tparam T The sample type to write.
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @param value The sample value.
    /// @throws InvalidParameterError when out of bounds.
    /// @throws UnsupportedFormatError on a size mismatch.
    template <typename T> void set(int32 x, int32 y, uint32 c, T value);

    /// Convenience byte accessor for 8-bit images (legacy-friendly).
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @return Reference to the sample byte.
    byte& operator()(int32 x, int32 y, uint32 c);
    /// Const convenience byte accessor for 8-bit images.
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @return The sample byte.
    byte operator()(int32 x, int32 y, uint32 c) const;

    /// Reads a sample as a normalized [0, 1] float, independent of storage type.
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @return The normalized sample value.
    /// @throws InvalidParameterError when out of bounds.
    /// @throws UnsupportedFormatError on unsupported data types.
    float readNorm(int32 x, int32 y, uint32 c) const;

    /// Reads the layout-aware canonical RGBA Pixel at (x, y).
    /// @param x Column index.
    /// @param y Row index.
    /// @return The canonical RGBA Pixel.
    /// @throws InvalidParameterError when out of bounds.
    /// @throws UnsupportedFormatError on unsupported pixel formats.
    Pixel readPixel(int32 x, int32 y) const;

    /// Stores a normalized [0, 1] value, converting to the storage type.
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @param value The normalized value to store.
    /// @throws InvalidParameterError when out of bounds.
    /// @throws UnsupportedFormatError on unsupported data types.
    void write(int32 x, int32 y, uint32 c, float value);

    /// Stores a canonical RGBA Pixel, honoring the layout and storage type.
    /// @param p The canonical RGBA Pixel.
    /// @param x Column index.
    /// @param y Row index.
    /// @throws InvalidParameterError when out of bounds.
    /// @throws UnsupportedFormatError on unsupported pixel formats.
    void write(const Pixel& p, int32 x, int32 y);

    /// Returns a mutable view over this image.
    /// @return A mutable ImageView.
    ImageView view() noexcept;
    /// Returns a const view over this image.
    /// @return A ConstImageView.
    ConstImageView view() const noexcept;
    /// Returns a const view over this image.
    /// @return A ConstImageView.
    ConstImageView cview() const noexcept;

    /// Crops in-place view semantics: returns an ImageView over this image.
    /// @param x0 Left offset.
    /// @param y0 Top offset.
    /// @param w Sub-view width.
    /// @param h Sub-view height.
    /// @return A mutable sub-view over this image.
    ImageView subView(uint32 x0, uint32 y0, uint32 w, uint32 h) noexcept;
    /// Returns a const sub-view over this image.
    /// @param x0 Left offset.
    /// @param y0 Top offset.
    /// @param w Sub-view width.
    /// @param h Sub-view height.
    /// @return A const sub-view over this image.
    ConstImageView subView(uint32 x0, uint32 y0, uint32 w, uint32 h) const noexcept;

  private:
    /// Allocates backing storage for the given layout.
    /// @param f Pixel format of the image.
    /// @param w Width in pixels.
    /// @param h Height in pixels.
    /// @param cs Color space.
    /// @param am Alpha mode.
    /// @param rowAlignment Row alignment in bytes.
    /// @throws InvalidParameterError on an invalid format or alignment.
    /// @throws InvalidDimensionError on zero dimensions or oversized buffers.
    /// @throws UnsupportedFormatError when more than 4 channels are requested.
    void
    allocate(ImageFormat f, uint32 w, uint32 h, ColorSpace cs, AlphaMode am, uint32 rowAlignment);

    /// Backing pixel memory; the image is the sole owner.
    mem::Buffer buffer_;
    /// Width in pixels.
    uint32 width_ = 0;
    /// Height in pixels.
    uint32 height_ = 0;
    /// Number of channels per pixel.
    uint16 channels_ = 0;
    /// Row stride in bytes.
    uint32 stride_ = 0;
    /// Pixel format and data type.
    ImageFormat format_;
    /// Color space of the samples.
    ColorSpace colorSpace_ = ColorSpace::Unknown;
    /// Alpha mode of the samples.
    AlphaMode alphaMode_ = AlphaMode::Unknown;
    /// Configured row alignment in bytes.
    uint32 rowAlign_ = defaultRowAlignment;
};

/// Swaps the contents of two images.
/// @param a First image.
/// @param b Second image.
void swap(Image& a, Image& b) noexcept;

} // namespace iml

#include "core/image/Image.tpp"