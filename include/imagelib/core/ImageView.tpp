// imagelib/core/ImageView.tpp
//
// Member definitions of the ImageViewBase class template. This file is
// included at the bottom of ImageView.h (never include it directly).

#ifndef IMAGELIB_CORE_IMAGEVIEW_H_
#error "Include ImageView.h, not ImageView.tpp directly."
#endif

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/core/memory/Memory.h"

#include <algorithm>

namespace iml {

// -- construction ------------------------------------------------------

template <bool Const>
inline ImageViewBase<Const>::ImageViewBase(BytePtr pixelData, uint32 w, uint32 h,
                                           uint16 ch, uint32 stride,
                                           const ImageFormat& f,
                                           ColorSpace cs, AlphaMode am) noexcept
    : data_(pixelData), width_(w), height_(h), channels_(ch),
      stride_(stride), format_(f), colorSpace_(cs), alphaMode_(am) {}

template <bool Const>
template <bool C2, std::enable_if_t<!C2 || Const, int>>
constexpr ImageViewBase<Const>::ImageViewBase(const ImageViewBase<C2>& other) noexcept
    : data_(other.data()), width_(other.width()), height_(other.height()),
      channels_(other.channels()), stride_(other.stride()),
      pixelStride_(other.pixelStride()), format_(other.format()),
      colorSpace_(other.colorSpace()), alphaMode_(other.alphaMode()) {}

// -- metadata ----------------------------------------------------------

template <bool Const>
inline bool ImageViewBase<Const>::valid() const noexcept {
    return data_ != nullptr && width_ > 0 && height_ > 0
        && channels_ > 0 && stride_ > 0 && format_.valid()
        && channels_ <= 4;
}

template <bool Const>
inline bool ImageViewBase<Const>::empty() const noexcept { return width_ == 0 || height_ == 0; }

template <bool Const>
inline uint32 ImageViewBase<Const>::width()  const noexcept { return width_; }
template <bool Const>
inline uint32 ImageViewBase<Const>::height() const noexcept { return height_; }
template <bool Const>
inline uint16 ImageViewBase<Const>::channels() const noexcept { return channels_; }
template <bool Const>
inline uint32 ImageViewBase<Const>::stride() const noexcept { return stride_; }
template <bool Const>
inline Size   ImageViewBase<Const>::size() const noexcept { return Size(width_, height_); }
template <bool Const>
inline const ImageFormat& ImageViewBase<Const>::format() const noexcept { return format_; }
template <bool Const>
inline PixelFormat ImageViewBase<Const>::pixelFormat() const noexcept { return format_.pixelFormat; }
template <bool Const>
inline DataType    ImageViewBase<Const>::dataType() const noexcept { return format_.dataType; }
template <bool Const>
inline ColorSpace  ImageViewBase<Const>::colorSpace() const noexcept { return colorSpace_; }
template <bool Const>
inline AlphaMode   ImageViewBase<Const>::alphaMode() const noexcept { return alphaMode_; }

template <bool Const>
inline uint32 ImageViewBase<Const>::pixelStride() const noexcept {
    return pixelStride_ ? pixelStride_ : static_cast<uint32>(channels_ * bytesPerChannel());
}

template <bool Const>
inline uint32 ImageViewBase<Const>::bytesPerChannel() const noexcept { return dataTypeSize(dataType()); }

template <bool Const>
inline uint32 ImageViewBase<Const>::pixelSizeBytes() const noexcept { return pixelStride(); }

template <bool Const>
inline uint32 ImageViewBase<Const>::rowPaddedSize() const noexcept { return stride_; }

template <bool Const>
inline Rect  ImageViewBase<Const>::bounds() const noexcept { return Rect(0, 0, width_, height_); }

// -- access ------------------------------------------------------------

template <bool Const>
inline typename ImageViewBase<Const>::BytePtr ImageViewBase<Const>::data() const noexcept { return data_; }

template <bool Const>
inline typename ImageViewBase<Const>::BytePtr ImageViewBase<Const>::row(int32 y) const noexcept {
    return data_ + static_cast<uint64>(y) * stride_;
}

template <bool Const>
inline typename ImageViewBase<Const>::BytePtr ImageViewBase<Const>::pixelAt(int32 x, int32 y) const noexcept {
    return row(y) + static_cast<uint64>(x) * pixelStride();
}

template <bool Const>
inline typename ImageViewBase<Const>::BytePtr ImageViewBase<Const>::sampleAt(int32 x, int32 y, uint32 c) const noexcept {
    return pixelAt(x, y) + static_cast<uint64>(c) * bytesPerChannel();
}

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

// -- sub-views (ROI / channel) -----------------------------------------

template <bool Const>
inline ImageViewBase<Const> ImageViewBase<Const>::subView(uint32 x0, uint32 y0,
                                                          uint32 w, uint32 h) const noexcept {
    if (!valid()) return ImageViewBase();
    const uint32 cx0 = std::min(x0, width_);
    const uint32 cy0 = std::min(y0, height_);
    const uint32 cw  = std::min(w, width_  - cx0);
    const uint32 ch  = std::min(h, height_ - cy0);
    if (cw == 0 || ch == 0) return ImageViewBase();
    return ImageViewBase(data_ + static_cast<uint64>(cy0) * stride_
                            + static_cast<uint64>(cx0) * pixelStride(),
                         cw, ch, channels_, stride_, format_,
                         colorSpace_, alphaMode_);
}

template <bool Const>
inline ImageViewBase<Const> ImageViewBase<Const>::channelView(uint32 c) const noexcept {
    if (!valid() || c >= channels_) return ImageViewBase();
    ImageViewBase v(data_ + static_cast<uint64>(c) * bytesPerChannel(),
                    width_, height_, 1, stride_,
                    ImageFormat(PixelFormat::Gray, format_.dataType),
                    colorSpace_, alphaMode_);
    v.pixelStride_ = pixelStride();
    return v;
}

template <bool Const>
inline bool ImageViewBase<Const>::operator!() const noexcept { return !valid(); }

template <bool Const>
inline void ImageViewBase<Const>::checkBounds(int32 x, int32 y, uint32 c) const {
    if (x < 0 || static_cast<uint32>(x) >= width_
        || y < 0 || static_cast<uint32>(y) >= height_
        || c >= channels_) {
        throw InvalidParameterError("ImageView: sample coordinates out of bounds");
    }
}

} // namespace iml