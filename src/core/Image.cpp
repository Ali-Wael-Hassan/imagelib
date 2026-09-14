// imagelib/src/core/Image.cpp

#include "imagelib/core/Image.h"
#include "imagelib/core/memory/Allocator.h"
#include "imagelib/codecs/Codec.h"

#include <algorithm>
#include <cstdint>

namespace iml {

namespace {

// Determine the image format descriptor for channel count + data type.
PixelFormat pixelFormatForChannels(uint16 ch) {
    switch (ch) {
        case 1:  return PixelFormat::Gray;
        case 2:  return PixelFormat::GrayAlpha;
        case 3:  return PixelFormat::RGB;
        case 4:  return PixelFormat::RGBA;
        default: throw InvalidParameterError("Image: invalid channel count (>4)");
    }
}

uint32 computeStride(uint32 width, uint16 channels, uint32 bpc, uint32 rowAlign) {
    uint64 rowBytes = static_cast<uint64>(width) * channels * bpc;
    if (rowAlign > 1) {
        rowBytes = mem::alignUp(rowBytes, rowAlign);
    }
    return static_cast<uint32>(rowBytes);
}

} // namespace

Image::Image(const std::string& path) {
    load(path);
}

Image::Image(const Image& other) {
    *this = other;
}

Image::Image(Image&& other) noexcept {
    swap(other);
}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) { release(); swap(other); }
    return *this;
}

void Image::release() noexcept {
    buffer_ = mem::Buffer();
    width_ = height_ = channels_ = stride_ = 0;
    format_ = ImageFormat();
    colorSpace_ = ColorSpace::Unknown;
    alphaMode_  = AlphaMode::Unknown;
}

bool Image::empty() const noexcept { return width_ == 0 || height_ == 0; }
uint32 Image::width()  const noexcept { return width_; }
uint32 Image::height() const noexcept { return height_; }
uint16 Image::channels() const noexcept { return channels_; }
uint32 Image::stride() const noexcept { return stride_; }
Size   Image::size() const noexcept { return Size(width_, height_); }
const ImageFormat& Image::format() const noexcept { return format_; }
PixelFormat Image::pixelFormat() const noexcept { return format_.pixelFormat; }
DataType    Image::dataType() const noexcept { return format_.dataType; }
ColorSpace  Image::colorSpace() const noexcept { return colorSpace_; }
AlphaMode   Image::alphaMode() const noexcept { return alphaMode_; }
uint32 Image::pixelSizeBytes() const noexcept { return bytesPerChannel() * channels_; }
uint32 Image::bytesPerChannel() const noexcept { return dataTypeSize(format_.dataType); }
uint32 Image::rowAlign() const noexcept { return rowAlign_; }

bool Image::hasAlpha() const noexcept {
    return channels_ == 2 || channels_ == 4
        && alphaMode_ != AlphaMode::None;
}

uint64 Image::bufferBytes() const noexcept { return stride_ * static_cast<uint64>(height_); }

byte* Image::data() noexcept { return buffer_.data(); }
const byte* Image::data() const noexcept { return buffer_.data(); }
byte* Image::row(int32 y) noexcept { return buffer_.data() + static_cast<uint64>(y) * stride_; }
const byte* Image::row(int32 y) const noexcept { return buffer_.data() + static_cast<uint64>(y) * stride_; }
byte* Image::pixel(int32 x, int32 y) noexcept { return row(y) + static_cast<uint64>(x) * pixelSizeBytes(); }
const byte* Image::pixel(int32 x, int32 y) const noexcept { return row(y) + static_cast<uint64>(x) * pixelSizeBytes(); }
byte* Image::sample(int32 x, int32 y, uint32 c) noexcept { return pixel(x, y) + static_cast<uint64>(c) * bytesPerChannel(); }
const byte* Image::sample(int32 x, int32 y, uint32 c) const noexcept { return pixel(x, y) + static_cast<uint64>(c) * bytesPerChannel(); }

byte& Image::operator()(int32 x, int32 y, uint32 c) {
    return *sample(x, y, c);
}

byte Image::operator()(int32 x, int32 y, uint32 c) const {
    return *sample(x, y, c);
}

ImageView Image::view() noexcept {
    return ImageView(data(), width_, height_, channels_, stride_, format_,
                     colorSpace_, alphaMode_);
}

ConstImageView Image::view() const noexcept {
    return ConstImageView(data(), width_, height_, channels_, stride_, format_,
                          colorSpace_, alphaMode_);
}

ConstImageView Image::cview() const noexcept { return view(); }

ImageView Image::subView(uint32 x0, uint32 y0, uint32 w, uint32 h) noexcept {
    return view().subView(x0, y0, w, h);
}

ConstImageView Image::subView(uint32 x0, uint32 y0, uint32 w, uint32 h) const noexcept {
    return view().subView(x0, y0, w, h);
}

void swap(Image& a, Image& b) noexcept { a.swap(b); }

void Image::allocate(ImageFormat f, uint32 w, uint32 h,
                     ColorSpace cs, AlphaMode am, uint32 rowAlignment) {
    if (!f.valid()) throw InvalidParameterError("Image: invalid image format");
    if (w == 0 || h == 0) throw InvalidDimensionError("Image: zero dimension");
    if (rowAlignment == 0 || !mem::isValidAlignment(rowAlignment)) {
        throw InvalidParameterError("Image: invalid row alignment");
    }
    if (f.channels() > 4) throw UnsupportedFormatError("Image: >4 channels requested");

    uint32 bpc    = dataTypeSize(f.dataType);
    uint32 stride = computeStride(w, f.channels(), bpc, rowAlignment);
    stride = std::max(stride, static_cast<uint32>(f.channels() * bpc));

    uint64 bytes = mem::mulChecked(stride, static_cast<uint64>(h));
    if (bytes > static_cast<uint64>(UINT32_MAX) * 4 + 4) {
        throw InvalidDimensionError("Image: buffer exceeds supported size");
    }

    // release any previous storage
    release();
    ColorSpace actualCS = cs;
    if (actualCS == ColorSpace::Unknown && f.channels() == 1) actualCS = ColorSpace::Gray;

    buffer_     = mem::Buffer(static_cast<size_t>(bytes),
                              std::max<size_t>(rowAlignment, mem::defaultAlignment),
                              &mem::DefaultAllocator::instance());
    width_      = w;
    height_     = h;
    channels_   = static_cast<uint16>(f.channels());
    stride_     = stride;
    format_     = f;
    colorSpace_ = actualCS;
    alphaMode_  = am;
    rowAlign_   = rowAlignment;
}

Image::Image(ImageFormat f, uint32 w, uint32 h, ColorSpace cs, AlphaMode am, uint32 rowAlignment) {
    allocate(f, w, h, cs, am, rowAlignment);
}

Image& Image::operator=(const Image& other) {
    if (this == &other) return *this;
    if (other.empty()) { release(); return *this; }

    // Reuse storage when the layout matches to avoid reallocation.
    if (buffer_.capacity() >= other.bufferBytes()
        && channels_ == other.channels_ && stride_ == other.stride_
        && format_ == other.format_) {
        mem::copy(buffer_.data(), other.buffer_.data(), static_cast<size_t>(other.bufferBytes()));
        colorSpace_ = other.colorSpace_;
        alphaMode_  = other.alphaMode_;
        return *this;
    }

    Image tmp(other.format_, other.width_, other.height_, other.colorSpace_,
              other.alphaMode_, other.rowAlign_);
    mem::copy(tmp.data(), other.data(), static_cast<size_t>(other.bufferBytes()));
    swap(tmp);
    return *this;
}

Image Image::clone() const { return *this; }

void Image::swap(Image& other) noexcept {
    buffer_.swap(other.buffer_);
    std::swap(width_,      other.width_);
    std::swap(height_,     other.height_);
    std::swap(channels_,   other.channels_);
    std::swap(stride_,     other.stride_);
    std::swap(format_,     other.format_);
    std::swap(colorSpace_, other.colorSpace_);
    std::swap(alphaMode_,  other.alphaMode_);
    std::swap(rowAlign_,   other.rowAlign_);
}

void Image::load(const std::string& path) {
    codecs::loadImage(*this, path);
}

void Image::save(const std::string& path, const SaveOptions& opts) const {
    codecs::saveImage(*this, path, opts);
}

} // namespace iml