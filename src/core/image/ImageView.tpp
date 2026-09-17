#ifndef IMAGELIB_CORE_IMAGEVIEW_H_
#error "Include ImageView.h, not ImageView.tpp directly."
#endif

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/core/memory/Memory.h"

#include <algorithm>

namespace iml {

/// Creates a view over external pixel memory.
/// @param pixelData Pointer to the first pixel of the data.
/// @param w Width in pixels.
/// @param h Height in pixels.
/// @param ch Number of channels per pixel.
/// @param stride Row stride in bytes.
/// @param f Pixel format and data type.
/// @param cs Color space.
/// @param am Alpha mode.
template <bool Const>
inline ImageViewBase<Const>::ImageViewBase(
    BytePtr pixelData,
    uint32 w,
    uint32 h,
    uint16 ch,
    uint32 stride,
    const ImageFormat& f,
    ColorSpace cs,
    AlphaMode am) noexcept
    : data_(pixelData), width_(w), height_(h), channels_(ch), stride_(stride), format_(f),
      colorSpace_(cs), alphaMode_(am) {}

/// Converts a mutable view to a const view (and identity copies).
/// @tparam C2 Constness of the source view.
/// @param other The source view.
template <bool Const>
template <bool C2, std::enable_if_t<!C2 || Const, int>>
constexpr ImageViewBase<Const>::ImageViewBase(const ImageViewBase<C2>& other) noexcept
    : data_(other.data()), width_(other.width()), height_(other.height()),
      channels_(other.channels()), stride_(other.stride()), pixelStride_(other.pixelStride()),
      format_(other.format()), colorSpace_(other.colorSpace()), alphaMode_(other.alphaMode()) {}

/// Returns true when the view wraps valid, non-empty memory.
/// @return True when valid and non-empty.
template <bool Const> inline bool ImageViewBase<Const>::valid() const noexcept {
    return data_ != nullptr && width_ > 0 && height_ > 0 && channels_ > 0 && stride_ > 0 &&
           format_.valid() && channels_ <= 4;
}

/// Returns true when the view has no pixels (zero width or height).
/// @return True when empty.
template <bool Const> inline bool ImageViewBase<Const>::empty() const noexcept {
    return width_ == 0 || height_ == 0;
}

/// Returns the width in pixels.
/// @return Width in pixels.
template <bool Const> inline uint32 ImageViewBase<Const>::width() const noexcept { return width_; }
/// Returns the height in pixels.
/// @return Height in pixels.
template <bool Const> inline uint32 ImageViewBase<Const>::height() const noexcept {
    return height_;
}
/// Returns the number of channels per pixel.
/// @return Channel count.
template <bool Const> inline uint16 ImageViewBase<Const>::channels() const noexcept {
    return channels_;
}
/// Returns the row stride in bytes.
/// @return Row stride in bytes.
template <bool Const> inline uint32 ImageViewBase<Const>::stride() const noexcept {
    return stride_;
}
/// Returns the width and height as a Size.
/// @return View size.
template <bool Const> inline Size ImageViewBase<Const>::size() const noexcept {
    return Size(width_, height_);
}
/// Returns the pixel format and data type.
/// @return The image format.
template <bool Const> inline const ImageFormat& ImageViewBase<Const>::format() const noexcept {
    return format_;
}
/// Returns the pixel format.
/// @return The pixel format.
template <bool Const> inline PixelFormat ImageViewBase<Const>::pixelFormat() const noexcept {
    return format_.pixelFormat;
}
/// Returns the sample data type.
/// @return The data type.
template <bool Const> inline DataType ImageViewBase<Const>::dataType() const noexcept {
    return format_.dataType;
}
/// Returns the color space.
/// @return The color space.
template <bool Const> inline ColorSpace ImageViewBase<Const>::colorSpace() const noexcept {
    return colorSpace_;
}
/// Returns the alpha mode.
/// @return The alpha mode.
template <bool Const> inline AlphaMode ImageViewBase<Const>::alphaMode() const noexcept {
    return alphaMode_;
}

/// Returns the bytes between two consecutive pixels.
/// @return Bytes between consecutive pixels.
template <bool Const> inline uint32 ImageViewBase<Const>::pixelStride() const noexcept {
    return pixelStride_ ? pixelStride_ : static_cast<uint32>(channels_ * bytesPerChannel());
}

/// Returns the number of bytes per sample channel.
/// @return Bytes per channel.
template <bool Const> inline uint32 ImageViewBase<Const>::bytesPerChannel() const noexcept {
    return dataTypeSize(dataType());
}

/// Returns the number of bytes per pixel.
/// @return Bytes per pixel.
template <bool Const> inline uint32 ImageViewBase<Const>::pixelSizeBytes() const noexcept {
    return pixelStride();
}

/// Returns the padded byte size of a single row.
/// @return Padded row size in bytes.
template <bool Const> inline uint32 ImageViewBase<Const>::rowPaddedSize() const noexcept {
    return stride_;
}

/// Returns the bounding rectangle of the view.
/// @return The bounds Rect.
template <bool Const> inline Rect ImageViewBase<Const>::bounds() const noexcept {
    return Rect(0, 0, width_, height_);
}

/// Returns the physical memory layout of this view.
/// @return The physical memory layout.
template <bool Const> inline PixelLayoutInfo ImageViewBase<Const>::memoryLayout() const noexcept {
    const PixelLayout l = layoutForPixelFormat(pixelFormat());
    PixelLayoutInfo info;
    info.layout = l;
    info.channels = static_cast<uint16>(channels_);
    info.bytes = bytesPerChannel();
    info.stride = l == PixelLayout::Planar ? stride_ : pixelStride();
    info.planeStride = 0;
    return info;
}

/// Returns a pointer to the first pixel byte.
/// @return Pointer to the view memory.
template <bool Const>
inline typename ImageViewBase<Const>::BytePtr ImageViewBase<Const>::data() const noexcept {
    return data_;
}

/// Returns a pointer to the first byte of row y.
/// @param y Row index.
/// @return Pointer to the row.
template <bool Const>
inline typename ImageViewBase<Const>::BytePtr ImageViewBase<Const>::row(int32 y) const noexcept {
    return data_ + static_cast<uint64>(y) * stride_;
}

/// Returns a pointer to the first byte of pixel (x, y).
/// @param x Column index.
/// @param y Row index.
/// @return Pointer to the pixel.
template <bool Const>
inline typename ImageViewBase<Const>::BytePtr
ImageViewBase<Const>::pixelAt(int32 x, int32 y) const noexcept {
    return row(y) + static_cast<uint64>(x) * pixelStride();
}

/// Returns a pointer to sample channel c of pixel (x, y).
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @return Pointer to the sample.
template <bool Const>
inline typename ImageViewBase<Const>::BytePtr
ImageViewBase<Const>::sampleAt(int32 x, int32 y, uint32 c) const noexcept {
    return pixelAt(x, y) + static_cast<uint64>(c) * bytesPerChannel();
}

/// Reads a single sample and converts it to T (widening casts included).
/// @tparam T The sample type to read.
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @return The sample as T.
/// @throws InvalidParameterError when out of bounds.
/// @throws UnsupportedFormatError on unsupported data types.
template <bool Const>
template <typename T>
inline T ImageViewBase<Const>::getSample(int32 x, int32 y, uint32 c) const {
    checkBounds(x, y, c);
    if (sizeof(T) == bytesPerChannel()) {
        T v;
        mem::copy(&v, sampleAt(x, y, c), sizeof(T));
        return v;
    }
    switch (dataType()) {
    case DataType::UInt8: {
        uint8 v;
        mem::copy(&v, sampleAt(x, y, c), sizeof(v));
        return static_cast<T>(v);
    }
    case DataType::UInt16: {
        uint16 v;
        mem::copy(&v, sampleAt(x, y, c), sizeof(v));
        return static_cast<T>(v);
    }
    case DataType::UInt32: {
        uint32 v;
        mem::copy(&v, sampleAt(x, y, c), sizeof(v));
        return static_cast<T>(v);
    }
    case DataType::Float32: {
        float v;
        mem::copy(&v, sampleAt(x, y, c), sizeof(v));
        return static_cast<T>(v);
    }
    default:
        break;
    }
    throw UnsupportedFormatError("ImageView: unsupported data type for getSample");
}

/// Writes a sample whose size matches bytesPerChannel() (mutable views).
/// @tparam T The sample type to write.
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @param value The sample value.
/// @throws InvalidParameterError when out of bounds.
/// @throws UnsupportedFormatError on a size mismatch.
template <bool Const>
template <typename T, bool C, std::enable_if_t<!C, int>>
inline void ImageViewBase<Const>::setSample(int32 x, int32 y, uint32 c, T value) {
    checkBounds(x, y, c);
    static_assert(sizeof(T) >= 1, "byte-sized sample required");
    if (sizeof(T) != bytesPerChannel()) {
        throw UnsupportedFormatError("ImageView: sample size mismatch in setSample");
    }
    mem::copy(sampleAt(x, y, c), &value, sizeof(T));
}

/// Returns a bounded sub-view; clamps to the parent view.
/// @param x0 Left offset.
/// @param y0 Top offset.
/// @param w Sub-view width.
/// @param h Sub-view height.
/// @return The bounded sub-view.
template <bool Const>
inline ImageViewBase<Const>
ImageViewBase<Const>::subView(uint32 x0, uint32 y0, uint32 w, uint32 h) const noexcept {
    if (!valid())
        return ImageViewBase();
    const uint32 cx0 = std::min(x0, width_);
    const uint32 cy0 = std::min(y0, height_);
    const uint32 cw = std::min(w, width_ - cx0);
    const uint32 ch = std::min(h, height_ - cy0);
    if (cw == 0 || ch == 0)
        return ImageViewBase();
    return ImageViewBase(
        data_ + static_cast<uint64>(cy0) * stride_ + static_cast<uint64>(cx0) * pixelStride(),
        cw,
        ch,
        channels_,
        stride_,
        format_,
        colorSpace_,
        alphaMode_);
}

/// Returns a single-channel sub-view over channel c.
/// @param c Channel index.
/// @return The channel sub-view.
template <bool Const>
inline ImageViewBase<Const> ImageViewBase<Const>::channelView(uint32 c) const noexcept {
    if (!valid() || c >= channels_)
        return ImageViewBase();
    ImageViewBase v(
        data_ + static_cast<uint64>(c) * bytesPerChannel(),
        width_,
        height_,
        1,
        stride_,
        ImageFormat(PixelFormat::Gray, format_.dataType),
        colorSpace_,
        alphaMode_);
    v.pixelStride_ = pixelStride();
    return v;
}

/// Returns true when the view is invalid.
/// @return True when invalid.
template <bool Const> inline bool ImageViewBase<Const>::operator!() const noexcept {
    return !valid();
}

/// Validates sample coordinates and throws when out of bounds.
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @throws InvalidParameterError when out of bounds.
template <bool Const>
inline void ImageViewBase<Const>::checkBounds(int32 x, int32 y, uint32 c) const {
    if (x < 0 || static_cast<uint32>(x) >= width_ || y < 0 || static_cast<uint32>(y) >= height_ ||
        c >= channels_) {
        throw InvalidParameterError("ImageView: sample coordinates out of bounds");
    }
}

} // namespace iml