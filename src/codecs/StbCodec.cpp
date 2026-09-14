// imagelib/src/codecs/StbCodec.cpp
//
// STB-backed decoder/encoder (PNG/JPEG/BMP/TGA).
//
// Ownership: stbi_load returns memory owned by stb_image; it is copied into an
// ImageLib-allocated Image and then released with stbi_image_free. Writes go
// through a tightly-packed temporary buffer owned here and released normally;
// the two allocators never mix.

#include "imagelib/codecs/Codec.h"
#include "imagelib/codecs/StbCodec.h"
#include "imagelib/core/memory/Buffer.h"
#include "imagelib/core/memory/Memory.h"
#include "imagelib/threading/ParallelFor.h"

#include <stb_image.h>
#include <stb_image_write.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace iml {
namespace codecs {

namespace {

constexpr int kDefaultJpegQuality = 90;

PixelFormat pixelFormatForChannels(uint16 ch) {
    switch (ch) {
        case 1:  return PixelFormat::Gray;
        case 2:  return PixelFormat::GrayAlpha;
        case 3:  return PixelFormat::RGB;
        case 4:  return PixelFormat::RGBA;
        default: return PixelFormat::Unknown;
    }
}

// ---------------------------------------------------------------------------
// STB codec implementation
// ---------------------------------------------------------------------------

class StbCodec : public ICodec {
public:
    StbCodec(ImageCodecFormat fmt, std::string ext)
        : info_{} {
        info_.format = fmt;
        info_.extension = std::move(ext);
        info_.readablePixels  = { PixelFormat::Gray, PixelFormat::GrayAlpha,
                                  PixelFormat::RGB, PixelFormat::BGR,
                                  PixelFormat::RGBA, PixelFormat::BGRA };
        info_.writablePixels  = { PixelFormat::Gray, PixelFormat::GrayAlpha,
                                  PixelFormat::RGB, PixelFormat::BGR,
                                  PixelFormat::RGBA, PixelFormat::BGRA };
    }

    const CodecInfo& info() const override { return info_; }

    bool canReadHeader(const byte* data, size_t size) const noexcept override {
        switch (info_.format) {
            case ImageCodecFormat::PNG:
                return size >= 8 && data[0] == 0x89 && data[1] == 'P'
                    && data[2] == 'N' && data[3] == 'G';
            case ImageCodecFormat::JPEG:
                return size >= 3 && data[0] == 0xFF && data[1] == 0xD8
                    && data[2] == 0xFF;
            case ImageCodecFormat::BMP:
                return size >= 2 && data[0] == 'B' && data[1] == 'M';
            default:
                return false;
        }
    }

    void read(Image& out, const std::string& path) override {
        int w = 0, h = 0, n = 0;
        unsigned char* px = stbi_load(path.c_str(), &w, &h, &n, 0);
        if (px == nullptr) {
            throw InvalidFileError("StbCodec::read: " + path);
        }

        const uint16 ch = static_cast<uint16>(n);
        const PixelFormat pf = pixelFormatForChannels(ch);
        try {
            if (pf == PixelFormat::Unknown) {
                throw UnsupportedFormatError("StbCodec::read: channel count " + std::to_string(n));
            }
            Image img(ImageFormat(pf, DataType::UInt8),
                      static_cast<uint32>(w), static_cast<uint32>(h),
                      pf == PixelFormat::Gray ? ColorSpace::Gray : ColorSpace::SRGB,
                      (ch == 2 || ch == 4) ? AlphaMode::Straight : AlphaMode::None);
            // Row-parallel copy of the decoded scanlines into the padded
            // Image rows. Threads advance into independent source/destination
            // regions, so the copy scales with the thread pool.
            const uint32 rowBytes = static_cast<uint32>(w) * ch;
            const uint32 height = static_cast<uint32>(h);
            parallelForRows(height, [&](size_t y) {
                mem::copy(img.row(static_cast<int32>(y)),
                          px + static_cast<uint64>(y) * rowBytes, rowBytes);
            });
            out = std::move(img);
        } catch (...) {
            stbi_image_free(px);
            throw;
        }
        stbi_image_free(px);
    }

    void write(const Image& src, const std::string& path,
               const SaveOptions& opts) override {
        if (src.empty()) throw InvalidParameterError("StbCodec::write: empty image");
        if (src.dataType() != DataType::UInt8) {
            throw UnsupportedFormatError("StbCodec::write: 8-bit only (STB backend)");
        }

        uint16 comp = 0;
        switch (info_.format) {
            case ImageCodecFormat::PNG: comp = src.channels(); break;
            case ImageCodecFormat::JPEG: comp = (src.channels() == 1) ? 1 : 3; break;
            // stb's BMP reader does not reliably accept its own 32-bit BMP
            // output, so always write 24-bit RGB (BMP has no alpha anyway).
            case ImageCodecFormat::BMP: comp = 3; break;
            case ImageCodecFormat::TGA: comp = (src.channels() == 1 || src.channels() == 3 || src.channels() == 4)
                                             ? src.channels() : 4; break;
            default: throw UnsupportedFormatError("StbCodec::write: unknown format");
        }

        // Convert into tightly-packed, canonical RGBA-order bytes for the
        // writer. Each output row is written independently, so the conversion
        // runs over the thread pool for large frames.
        uint64 packed = mem::mulChecked(static_cast<uint64>(comp),
                                        mem::mulChecked(src.width(), static_cast<uint64>(src.height())));
        mem::Buffer buf(static_cast<size_t>(packed));
        byte* dst = buf.data();
        const ConstImageView view = src.view();
        const uint32 iw = src.width(), ih = src.height();

        parallelForRows(ih, [&](size_t row) {
            const uint32 y = static_cast<uint32>(row);
            for (uint32 x = 0; x < iw; ++x) {
                byte* o = dst + (static_cast<uint64>(y) * iw + x) * comp;
                uint8 v[4];
                uint8 a = 255;
                switch (view.pixelFormat()) {
                    case PixelFormat::Gray:
                        v[0] = v[1] = v[2] = *view.sampleAt(x, y, 0); break;
                    case PixelFormat::GrayAlpha:
                        v[0] = v[1] = v[2] = *view.sampleAt(x, y, 0); a = *view.sampleAt(x, y, 1); break;
                    case PixelFormat::RGB:
                        v[0] = *view.sampleAt(x, y, 0); v[1] = *view.sampleAt(x, y, 1);
                        v[2] = *view.sampleAt(x, y, 2); break;
                    case PixelFormat::BGR:
                        v[0] = *view.sampleAt(x, y, 2); v[1] = *view.sampleAt(x, y, 1);
                        v[2] = *view.sampleAt(x, y, 0); break;
                    case PixelFormat::RGBA:
                        v[0] = *view.sampleAt(x, y, 0); v[1] = *view.sampleAt(x, y, 1);
                        v[2] = *view.sampleAt(x, y, 2); a = *view.sampleAt(x, y, 3); break;
                    case PixelFormat::BGRA:
                        v[0] = *view.sampleAt(x, y, 2); v[1] = *view.sampleAt(x, y, 1);
                        v[2] = *view.sampleAt(x, y, 0); a = *view.sampleAt(x, y, 3); break;
                    default:
                        throw UnsupportedFormatError("StbCodec::write: unsupported pixel format");
                }
                switch (comp) {
                    case 1:
                        o[0] = static_cast<byte>((77 * v[0] + 150 * v[1] + 29 * v[2]) / 256);
                        break;
                    case 2:
                        o[0] = static_cast<byte>((77 * v[0] + 150 * v[1] + 29 * v[2]) / 256);
                        o[1] = a;
                        break;
                    case 3:
                        o[0] = v[0]; o[1] = v[1]; o[2] = v[2]; break;
                    case 4:
                        o[0] = v[0]; o[1] = v[1]; o[2] = v[2]; o[3] = a; break;
                }
            }
        });

        int ok = 0;
        switch (info_.format) {
            case ImageCodecFormat::PNG:
                ok = stbi_write_png(path.c_str(), static_cast<int>(src.width()),
                                    static_cast<int>(src.height()), comp, dst, 0);
                break;
            case ImageCodecFormat::JPEG:
                ok = stbi_write_jpg(path.c_str(), static_cast<int>(src.width()),
                                    static_cast<int>(src.height()), comp, dst,
                                    opts.jpegQuality > 0 ? opts.jpegQuality : kDefaultJpegQuality);
                break;
            case ImageCodecFormat::BMP:
                ok = stbi_write_bmp(path.c_str(), static_cast<int>(src.width()),
                                    static_cast<int>(src.height()), comp, dst);
                break;
            case ImageCodecFormat::TGA:
                ok = stbi_write_tga(path.c_str(), static_cast<int>(src.width()),
                                    static_cast<int>(src.height()), comp, dst);
                break;
            default:
                throw UnsupportedFormatError("StbCodec::write: unknown format");
        }
        if (!ok) throw CodecError("StbCodec::write: failed writing: " + path);
    }

private:
    CodecInfo info_;
};

} // namespace

void registerStbCodec() {
    static bool registered = [] {
        CodecRegistry& reg = CodecRegistry::instance();
        reg.add(new StbCodec(ImageCodecFormat::PNG, "png"));
        reg.add(new StbCodec(ImageCodecFormat::JPEG, "jpg"));
        reg.add(new StbCodec(ImageCodecFormat::JPEG, "jpeg"));
        reg.add(new StbCodec(ImageCodecFormat::BMP, "bmp"));
        reg.add(new StbCodec(ImageCodecFormat::TGA, "tga"));
        return true;
    }();
    (void)registered;
}

} // namespace codecs
} // namespace iml