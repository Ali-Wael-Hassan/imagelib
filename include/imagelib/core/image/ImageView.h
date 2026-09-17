#pragma once
#define IMAGELIB_CORE_IMAGEVIEW_H_

/// @file ImageView.h
/// Non-owning views over external pixel memory; pixels are stored row-major,
/// and ConstImageView reads are safe from multiple threads.

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/core/memory/Memory.h"

#include <type_traits>
#include <stdexcept>
#include <algorithm>

namespace iml {

/// Non-owning view over external pixel memory; Const selects const access.
template <bool Const> class ImageViewBase {
  public:
    /// Pointer type honoring the constness of the view.
    using BytePtr = std::conditional_t<Const, const byte*, byte*>;

    /// Constructs an empty (invalid) view.
    constexpr ImageViewBase() noexcept = default;

    /// Creates a view over external pixel memory.
    /// @param pixelData Pointer to the first pixel of the data.
    /// @param w Width in pixels.
    /// @param h Height in pixels.
    /// @param ch Number of channels per pixel.
    /// @param stride Row stride in bytes.
    /// @param f Pixel format and data type.
    /// @param cs Color space.
    /// @param am Alpha mode.
    ImageViewBase(
        BytePtr pixelData,
        uint32 w,
        uint32 h,
        uint16 ch,
        uint32 stride,
        const ImageFormat& f,
        ColorSpace cs = ColorSpace::Unknown,
        AlphaMode am = AlphaMode::Unknown) noexcept;

    /// Conversion from mutable -> const view (and identity copies).
    /// @tparam C2 Constness of the source view.
    /// @param other The source view.
    template <bool C2, std::enable_if_t<!C2 || Const, int> = 0>
    constexpr ImageViewBase(const ImageViewBase<C2>& other) noexcept;

    /// Returns true when the view wraps valid, non-empty memory.
    /// @return True when valid and non-empty.
    bool valid() const noexcept;
    /// Returns true when the view has no pixels (zero width or height).
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
    /// @return View size.
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

    /// Bytes between two consecutive pixels (may be > channels*bpc when the
    /// pixel stride is explicit, e.g. channel sub-views).
    /// @return Bytes between consecutive pixels.
    uint32 pixelStride() const noexcept;
    /// Returns the number of bytes per sample channel.
    /// @return Bytes per channel.
    uint32 bytesPerChannel() const noexcept;
    /// Returns the number of bytes per pixel.
    /// @return Bytes per pixel.
    uint32 pixelSizeBytes() const noexcept;
    /// Returns the padded byte size of a single row.
    /// @return Padded row size in bytes.
    uint32 rowPaddedSize() const noexcept;

    /// Returns the bounding rectangle of the view.
    /// @return The bounds Rect.
    Rect bounds() const noexcept;

    /// Physical memory layout of this view. Interleaved/Gray are derived from
    /// the pixel format; planar views are never implied by a format, so this
    /// never returns Planar unless the view was constructed for it.
    /// @return The physical memory layout.
    PixelLayoutInfo memoryLayout() const noexcept;

    /// Returns a pointer to the first pixel byte.
    /// @return Pointer to the view memory.
    BytePtr data() const noexcept;
    /// Returns a pointer to the first byte of row y.
    /// @param y Row index.
    /// @return Pointer to the row.
    BytePtr row(int32 y) const noexcept;
    /// Returns a pointer to the first byte of pixel (x, y).
    /// @param x Column index.
    /// @param y Row index.
    /// @return Pointer to the pixel.
    BytePtr pixelAt(int32 x, int32 y) const noexcept;
    /// Returns a pointer to sample channel c of pixel (x, y).
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @return Pointer to the sample.
    BytePtr sampleAt(int32 x, int32 y, uint32 c) const noexcept;

    /// Reads a single sample and converts it to T (widening casts included).
    /// @tparam T The sample type to read.
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @return The sample as T.
    /// @throws InvalidParameterError when out of bounds.
    /// @throws UnsupportedFormatError on unsupported data types.
    template <typename T> T getSample(int32 x, int32 y, uint32 c) const;

    /// Writes a sample whose size matches bytesPerChannel() (mutable views).
    /// @tparam T The sample type to write.
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @param value The sample value.
    /// @throws InvalidParameterError when out of bounds.
    /// @throws UnsupportedFormatError on a size mismatch.
    template <typename T, bool C = Const, std::enable_if_t<!C, int> = 0>
    void setSample(int32 x, int32 y, uint32 c, T value);

    /// Bounded sub-view; clamps to parent so no dangling views are formed.
    /// @param x0 Left offset.
    /// @param y0 Top offset.
    /// @param w Sub-view width.
    /// @param h Sub-view height.
    /// @return The bounded sub-view.
    ImageViewBase subView(uint32 x0, uint32 y0, uint32 w, uint32 h) const noexcept;

    /// Single-channel sub-view over channel `c`. Keeps the underlying pixel
    /// stride so getSample(x, y, 0) addresses the correct samples.
    /// @param c Channel index.
    /// @return The channel sub-view.
    ImageViewBase channelView(uint32 c) const noexcept;

    /// Returns true when the view is invalid.
    /// @return True when invalid.
    bool operator!() const noexcept;

  protected:
    /// Validates sample coordinates and throws when out of bounds.
    /// @param x Column index.
    /// @param y Row index.
    /// @param c Channel index.
    /// @throws InvalidParameterError when out of bounds.
    void checkBounds(int32 x, int32 y, uint32 c) const;

    /// Pointer to the first pixel byte.
    BytePtr data_ = nullptr;
    /// Width in pixels.
    uint32 width_ = 0;
    /// Height in pixels.
    uint32 height_ = 0;
    /// Number of channels per pixel.
    uint16 channels_ = 0;
    /// Row stride in bytes.
    uint32 stride_ = 0;
    /// Explicit pixel stride in bytes; 0 means channels * bytesPerChannel.
    uint32 pixelStride_ = 0;
    /// Pixel format and data type.
    ImageFormat format_;
    /// Color space of the samples.
    ColorSpace colorSpace_ = ColorSpace::Unknown;
    /// Alpha mode of the samples.
    AlphaMode alphaMode_ = AlphaMode::Unknown;
};

/// Mutable view over external pixel memory.
using ImageView = ImageViewBase<false>;
/// Const view over external pixel memory; safe to read from multiple threads.
using ConstImageView = ImageViewBase<true>;

} // namespace iml

#include "core/image/ImageView.tpp"