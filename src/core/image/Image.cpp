#include "imagelib/core/image/Image.h"
#include "imagelib/core/memory/Allocator.h"
#include "imagelib/codecs/Codec.h"
#include "ImageUtils.h"

#include <algorithm>
#include <cstdint>

namespace iml {

/// Loads an image from the given path.
/// @param path The source image file.
/// @throws Codec or format errors from the codec layer.
Image::Image(const std::string& path) { load(path); }

/// Copy constructor; delegates to copy assignment (deep copy).
/// @param other The source image.
Image::Image(const Image& other) { *this = other; }

/// Move constructor; transfers ownership from another image.
/// @param other The source image.
Image::Image(Image&& other) noexcept { swap(other); }

/// Move assignment; releases current storage and transfers ownership.
/// @param other The source image.
/// @return Reference to this image.
Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        release();
        swap(other);
    }
    return *this;
}

/// Releases pixel memory; the instance becomes empty and still valid.
void Image::release() noexcept {
    buffer_ = mem::Buffer();
    width_ = height_ = channels_ = stride_ = 0;
    format_ = ImageFormat();
    colorSpace_ = ColorSpace::Unknown;
    alphaMode_ = AlphaMode::Unknown;
}

/// Returns true when the image has no pixels.
/// @return True when empty.
bool Image::empty() const noexcept { return width_ == 0 || height_ == 0; }
/// Returns the width in pixels.
/// @return Width in pixels.
uint32 Image::width() const noexcept { return width_; }
/// Returns the height in pixels.
/// @return Height in pixels.
uint32 Image::height() const noexcept { return height_; }
/// Returns the number of channels per pixel.
/// @return Channel count.
uint16 Image::channels() const noexcept { return channels_; }
/// Returns the row stride in bytes.
/// @return Row stride in bytes.
uint32 Image::stride() const noexcept { return stride_; }
/// Returns the width and height as a Size.
/// @return Image size.
Size Image::size() const noexcept { return Size(width_, height_); }
/// Returns the pixel format and data type.
/// @return The image format.
const ImageFormat& Image::format() const noexcept { return format_; }
/// Returns the pixel format.
/// @return The pixel format.
PixelFormat Image::pixelFormat() const noexcept { return format_.pixelFormat; }
/// Returns the sample data type.
/// @return The data type.
DataType Image::dataType() const noexcept { return format_.dataType; }
/// Returns the color space.
/// @return The color space.
ColorSpace Image::colorSpace() const noexcept { return colorSpace_; }
/// Returns the alpha mode.
/// @return The alpha mode.
AlphaMode Image::alphaMode() const noexcept { return alphaMode_; }
/// Returns the number of bytes per pixel.
/// @return Bytes per pixel.
uint32 Image::pixelSizeBytes() const noexcept { return bytesPerChannel() * channels_; }
/// Returns the number of bytes per channel.
/// @return Bytes per channel.
uint32 Image::bytesPerChannel() const noexcept { return dataTypeSize(format_.dataType); }
/// Returns the configured row alignment in bytes.
/// @return Row alignment in bytes.
uint32 Image::rowAlign() const noexcept { return rowAlign_; }

/// Returns true when the layout stores an alpha channel.
/// @return True when alpha is present.
bool Image::hasAlpha() const noexcept {
    return channels_ == 2 || channels_ == 4 && alphaMode_ != AlphaMode::None;
}

/// Returns the byte size of the backing buffer (stride * height).
/// @return Backing buffer size in bytes.
uint64 Image::bufferBytes() const noexcept { return stride_ * static_cast<uint64>(height_); }

/// Returns a pointer to the first pixel byte.
/// @return Pointer to the backing buffer.
byte* Image::data() noexcept { return buffer_.data(); }
/// Returns a const pointer to the first pixel byte.
/// @return Const pointer to the backing buffer.
const byte* Image::data() const noexcept { return buffer_.data(); }
/// Returns a pointer to the first byte of row y.
/// @param y Row index.
/// @return Pointer to the row.
byte* Image::row(int32 y) noexcept { return buffer_.data() + static_cast<uint64>(y) * stride_; }
/// Returns a const pointer to the first byte of row y.
/// @param y Row index.
/// @return Const pointer to the row.
const byte* Image::row(int32 y) const noexcept {
    return buffer_.data() + static_cast<uint64>(y) * stride_;
}
/// Returns a pointer to the first byte of pixel (x, y).
/// @param x Column index.
/// @param y Row index.
/// @return Pointer to the pixel.
byte* Image::pixel(int32 x, int32 y) noexcept {
    return row(y) + static_cast<uint64>(x) * pixelSizeBytes();
}
/// Returns a const pointer to the first byte of pixel (x, y).
/// @param x Column index.
/// @param y Row index.
/// @return Const pointer to the pixel.
const byte* Image::pixel(int32 x, int32 y) const noexcept {
    return row(y) + static_cast<uint64>(x) * pixelSizeBytes();
}
/// Returns a pointer to sample channel c of pixel (x, y).
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @return Pointer to the sample.
byte* Image::sample(int32 x, int32 y, uint32 c) noexcept {
    return pixel(x, y) + static_cast<uint64>(c) * bytesPerChannel();
}
/// Returns a const pointer to sample channel c of pixel (x, y).
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @return Const pointer to the sample.
const byte* Image::sample(int32 x, int32 y, uint32 c) const noexcept {
    return pixel(x, y) + static_cast<uint64>(c) * bytesPerChannel();
}

/// Returns the sample byte of channel c of pixel (x, y).
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @return Reference to the sample byte.
byte& Image::operator()(int32 x, int32 y, uint32 c) { return *sample(x, y, c); }

/// Returns the sample byte of channel c of pixel (x, y).
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @return The sample byte.
byte Image::operator()(int32 x, int32 y, uint32 c) const { return *sample(x, y, c); }

/// Reads a sample as a normalized [0, 1] float.
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @return The normalized sample value.
float Image::readNorm(int32 x, int32 y, uint32 c) const { return pixel::readNorm(view(), x, y, c); }

/// Reads the layout-aware canonical RGBA Pixel at (x, y).
/// @param x Column index.
/// @param y Row index.
/// @return The canonical RGBA Pixel.
Pixel Image::readPixel(int32 x, int32 y) const { return pixel::readPixel(view(), x, y); }

/// Stores a normalized [0, 1] value, converting to the storage type.
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @param value The normalized value to store.
void Image::write(int32 x, int32 y, uint32 c, float value) {
    pixel::writeNorm(view(), x, y, c, value);
}

/// Stores a canonical RGBA Pixel, honoring the layout and storage type.
/// @param p The canonical RGBA Pixel.
/// @param x Column index.
/// @param y Row index.
void Image::write(const Pixel& p, int32 x, int32 y) { pixel::writeRGBA(p, view(), x, y); }

/// Returns a mutable view over this image.
/// @return A mutable ImageView.
ImageView Image::view() noexcept {
    return ImageView(data(), width_, height_, channels_, stride_, format_, colorSpace_, alphaMode_);
}

/// Returns a const view over this image.
/// @return A ConstImageView.
ConstImageView Image::view() const noexcept {
    return ConstImageView(
        data(),
        width_,
        height_,
        channels_,
        stride_,
        format_,
        colorSpace_,
        alphaMode_);
}

/// Returns a const view over this image.
/// @return A ConstImageView.
ConstImageView Image::cview() const noexcept { return view(); }

/// Returns a mutable sub-view over this image.
/// @param x0 Left offset.
/// @param y0 Top offset.
/// @param w Sub-view width.
/// @param h Sub-view height.
/// @return A mutable sub-view over this image.
ImageView Image::subView(uint32 x0, uint32 y0, uint32 w, uint32 h) noexcept {
    return view().subView(x0, y0, w, h);
}

/// Returns a const sub-view over this image.
/// @param x0 Left offset.
/// @param y0 Top offset.
/// @param w Sub-view width.
/// @param h Sub-view height.
/// @return A const sub-view over this image.
ConstImageView Image::subView(uint32 x0, uint32 y0, uint32 w, uint32 h) const noexcept {
    return view().subView(x0, y0, w, h);
}

/// Swaps the contents of two images.
/// @param a First image.
/// @param b Second image.
void swap(Image& a, Image& b) noexcept { a.swap(b); }

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
void Image::allocate(
    ImageFormat f,
    uint32 w,
    uint32 h,
    ColorSpace cs,
    AlphaMode am,
    uint32 rowAlignment) {
    if (!f.valid())
        throw InvalidParameterError("Image: invalid image format");
    if (w == 0 || h == 0)
        throw InvalidDimensionError("Image: zero dimension");
    if (rowAlignment == 0 || !mem::isValidAlignment(rowAlignment)) {
        throw InvalidParameterError("Image: invalid row alignment");
    }
    if (f.channels() > 4)
        throw UnsupportedFormatError("Image: >4 channels requested");

    uint32 bpc = dataTypeSize(f.dataType);
    uint32 stride = computeStride(w, f.channels(), bpc, rowAlignment);
    stride = std::max(stride, static_cast<uint32>(f.channels() * bpc));

    uint64 bytes = mem::mulChecked(stride, static_cast<uint64>(h));
    if (bytes > static_cast<uint64>(UINT32_MAX) * 4 + 4) {
        throw InvalidDimensionError("Image: buffer exceeds supported size");
    }

    release();
    ColorSpace actualCS = cs;
    if (actualCS == ColorSpace::Unknown && f.channels() == 1)
        actualCS = ColorSpace::Gray;

    buffer_ = mem::Buffer(
        static_cast<size_t>(bytes),
        std::max<size_t>(rowAlignment, mem::defaultAlignment),
        &mem::DefaultAllocator::instance());
    width_ = w;
    height_ = h;
    channels_ = static_cast<uint16>(f.channels());
    stride_ = stride;
    format_ = f;
    colorSpace_ = actualCS;
    alphaMode_ = am;
    rowAlign_ = rowAlignment;
}

/// Allocates an empty (uninitialized) w x h image.
/// @param f Pixel format of the image.
/// @param w Width in pixels.
/// @param h Height in pixels.
/// @param cs Color space.
/// @param am Alpha mode.
/// @param rowAlignment Row alignment in bytes.
Image::Image(ImageFormat f, uint32 w, uint32 h, ColorSpace cs, AlphaMode am, uint32 rowAlignment) {
    allocate(f, w, h, cs, am, rowAlignment);
}

/// Deep-copy assignment.
/// @param other The source image.
/// @return Reference to this image.
Image& Image::operator=(const Image& other) {
    if (this == &other)
        return *this;
    if (other.empty()) {
        release();
        return *this;
    }

    if (buffer_.capacity() >= other.bufferBytes() && channels_ == other.channels_ &&
        stride_ == other.stride_ && format_ == other.format_) {
        mem::copy(buffer_.data(), other.buffer_.data(), static_cast<size_t>(other.bufferBytes()));
        colorSpace_ = other.colorSpace_;
        alphaMode_ = other.alphaMode_;
        return *this;
    }

    Image tmp(
        other.format_,
        other.width_,
        other.height_,
        other.colorSpace_,
        other.alphaMode_,
        other.rowAlign_);
    mem::copy(tmp.data(), other.data(), static_cast<size_t>(other.bufferBytes()));
    swap(tmp);
    return *this;
}

/// Returns a deep copy of this image.
/// @return A deep copy of this image.
Image Image::clone() const { return *this; }

/// Swaps the contents of this image with another.
/// @param other The image to swap with.
void Image::swap(Image& other) noexcept {
    buffer_.swap(other.buffer_);
    std::swap(width_, other.width_);
    std::swap(height_, other.height_);
    std::swap(channels_, other.channels_);
    std::swap(stride_, other.stride_);
    std::swap(format_, other.format_);
    std::swap(colorSpace_, other.colorSpace_);
    std::swap(alphaMode_, other.alphaMode_);
    std::swap(rowAlign_, other.rowAlign_);
}

/// Loads an image from the given path through the codec registry.
/// @param path The source image file.
/// @throws Codec or format errors from the codec layer.
void Image::load(const std::string& path) { codecs::loadImage(*this, path); }

/// Saves the image to the given path through the codec registry.
/// @param path The destination image file.
/// @param opts Save options.
/// @throws Codec or format errors from the codec layer.
void Image::save(const std::string& path, const SaveOptions& opts) const {
    codecs::saveImage(*this, path, opts);
}

} // namespace iml