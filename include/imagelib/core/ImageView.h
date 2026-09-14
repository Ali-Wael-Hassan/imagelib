#pragma once
#define IMAGELIB_CORE_IMAGEVIEW_H_
// imagelib/core/ImageView.h
//
// Non-owning view over externally-owned image memory. Processing algorithms
// accept ImageView / ConstImageView to operate in place without copying.
//
// Ownership: ImageView never owns, allocates or frees memory. The caller must
// keep the underlying memory alive for the lifetime of the view. Views are
// trivially copyable.
//
// Thread safety: reading a ConstImageView from multiple threads is safe;
// mutating through a mutable view from multiple threads is the caller's
// responsibility.
//
// Layout: pixels are stored row-major. Pixel i in row y begins at
//   data + y*stride + x*pixelStride
// and channel c of that pixel at + c*bytesPerChannel. This design lets ROI
// and channel sub-views stay compact and correct.

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/core/memory/Memory.h"

#include <type_traits>
#include <stdexcept>
#include <algorithm>

namespace iml {

template <bool Const>
class ImageViewBase {
public:
    using BytePtr = std::conditional_t<Const, const byte*, byte*>;

    // -- construction ------------------------------------------------------

    constexpr ImageViewBase() noexcept = default;

    ImageViewBase(BytePtr pixelData, uint32 w, uint32 h, uint16 ch,
                  uint32 stride, const ImageFormat& f,
                  ColorSpace cs = ColorSpace::Unknown,
                  AlphaMode am = AlphaMode::Unknown) noexcept;

    /// Conversion from mutable -> const view (and identity copies).
    template <bool C2, std::enable_if_t<!C2 || Const, int> = 0>
    constexpr ImageViewBase(const ImageViewBase<C2>& other) noexcept;

    // -- metadata ----------------------------------------------------------

    bool valid() const noexcept;
    bool empty() const noexcept;

    uint32 width()  const noexcept;
    uint32 height() const noexcept;
    uint16 channels() const noexcept;
    uint32 stride() const noexcept;
    Size   size() const noexcept;
    const ImageFormat& format() const noexcept;
    PixelFormat pixelFormat() const noexcept;
    DataType    dataType() const noexcept;
    ColorSpace  colorSpace() const noexcept;
    AlphaMode   alphaMode() const noexcept;

    /// Bytes between two consecutive pixels (may be > channels*bpc when the
    /// pixel stride is explicit, e.g. channel sub-views).
    uint32 pixelStride() const noexcept;
    uint32 bytesPerChannel() const noexcept;
    uint32 pixelSizeBytes() const noexcept;
    uint32 rowPaddedSize() const noexcept;

    Rect  bounds() const noexcept;

    // -- access ------------------------------------------------------------

    BytePtr data() const noexcept;
    BytePtr row(int32 y) const noexcept;
    BytePtr pixelAt(int32 x, int32 y) const noexcept;
    BytePtr sampleAt(int32 x, int32 y, uint32 c) const noexcept;

    /// Reads a single sample and converts it to T (widening casts included).
    template <typename T>
    T getSample(int32 x, int32 y, uint32 c) const;

    /// Writes a sample whose size matches bytesPerChannel() (mutable views).
    template <typename T, bool C = Const, std::enable_if_t<!C, int> = 0>
    void setSample(int32 x, int32 y, uint32 c, T value);

    // -- sub-views (ROI / channel) -----------------------------------------

    /// Bounded sub-view; clamps to parent so no dangling views are formed.
    ImageViewBase subView(uint32 x0, uint32 y0, uint32 w, uint32 h) const noexcept;

    /// Single-channel sub-view over channel `c`. Keeps the underlying pixel
    /// stride so getSample(x, y, 0) addresses the correct samples.
    ImageViewBase channelView(uint32 c) const noexcept;

    bool operator!() const noexcept;

protected:
    void checkBounds(int32 x, int32 y, uint32 c) const;

    BytePtr     data_       = nullptr;
    uint32      width_      = 0;
    uint32      height_     = 0;
    uint16      channels_   = 0;
    uint32      stride_     = 0;
    uint32      pixelStride_ = 0; // 0 => channels_ * bytesPerChannel
    ImageFormat format_;
    ColorSpace  colorSpace_ = ColorSpace::Unknown;
    AlphaMode   alphaMode_  = AlphaMode::Unknown;
};

using ImageView      = ImageViewBase<false>;
using ConstImageView = ImageViewBase<true>;

} // namespace iml

#include "imagelib/core/ImageView.tpp"