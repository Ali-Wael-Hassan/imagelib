#include "imagelib/codecs/Codec.h"
#include "imagelib/codecs/stb/Stb.h"
#include "imagelib/core/memory/Buffer.h"
#include "imagelib/core/memory/Memory.h"
#include "imagelib/threading/ParallelFor.h"

#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace iml {
namespace codecs {

namespace {

/// Default JPEG quality used when opts.jpegQuality is not set.
constexpr int kDefaultJpegQuality = 90;

void appendBe32(std::vector<byte>& out, uint32 value) {
    out.push_back(static_cast<byte>((value >> 24) & 0xffu));
    out.push_back(static_cast<byte>((value >> 16) & 0xffu));
    out.push_back(static_cast<byte>((value >> 8) & 0xffu));
    out.push_back(static_cast<byte>(value & 0xffu));
}

uint32 crc32(const byte* data, size_t size) {
    static const std::vector<uint32> table = [] {
        std::vector<uint32> values(256);
        for (uint32 i = 0; i < values.size(); ++i) {
            uint32 value = i;
            for (int bit = 0; bit < 8; ++bit)
                value = (value >> 1) ^ (0xedb88320u & static_cast<uint32>(-(value & 1u)));
            values[i] = value;
        }
        return values;
    }();
    uint32 crc = 0xffffffffu;
    for (size_t i = 0; i < size; ++i)
        crc = table[(crc ^ data[i]) & 0xffu] ^ (crc >> 8);
    return ~crc;
}

uint32 adler32(const std::vector<byte>& data) {
    uint32 a = 1, b = 0;
    for (byte value : data) {
        a += value;
        if (a >= 65521u)
            a -= 65521u;
        b += a;
        if (b >= 65521u)
            b -= 65521u;
    }
    return (b << 16) | a;
}

void appendChunk(std::vector<byte>& png, const char (&type)[5], const std::vector<byte>& data) {
    appendBe32(png, static_cast<uint32>(data.size()));
    const size_t typeOffset = png.size();
    png.insert(png.end(), type, type + 4);
    png.insert(png.end(), data.begin(), data.end());
    appendBe32(png, crc32(png.data() + typeOffset, 4 + data.size()));
}

bool writeFastPng(
    const std::string& path,
    uint32 width,
    uint32 height,
    uint16 channels,
    const byte* pixels) {
    const size_t rowBytes = static_cast<size_t>(width) * channels;
    std::vector<std::vector<byte>> rows(height);
    parallelForRows(
        height,
        [&](size_t y) {
            rows[y].resize(rowBytes + 1);
            rows[y][0] = 0;
            mem::copy(rows[y].data() + 1, pixels + y * rowBytes, rowBytes);
        },
        ExecutionPolicy::parallel());

    std::vector<byte> zlib;
    zlib.reserve(2 + static_cast<size_t>(height) * (rowBytes + 5) + 4);
    zlib.push_back(0x78);
    zlib.push_back(0x01);
    std::vector<byte> raw;
    raw.reserve(static_cast<size_t>(height) * (rowBytes + 1));
    for (const auto& row : rows)
        raw.insert(raw.end(), row.begin(), row.end());

    for (size_t y = 0; y < rows.size(); ++y) {
        const bool finalBlock = y + 1 == rows.size();
        zlib.push_back(finalBlock ? 0x01 : 0x00);
        const uint16 length = static_cast<uint16>(rows[y].size());
        zlib.push_back(static_cast<byte>(length & 0xffu));
        zlib.push_back(static_cast<byte>(length >> 8));
        const uint16 inverse = static_cast<uint16>(~length);
        zlib.push_back(static_cast<byte>(inverse & 0xffu));
        zlib.push_back(static_cast<byte>(inverse >> 8));
        zlib.insert(zlib.end(), rows[y].begin(), rows[y].end());
    }
    const uint32 adler = adler32(raw);
    appendBe32(zlib, adler);

    std::vector<byte> png;
    png.reserve(8 + 25 + zlib.size() + 12);
    const byte signature[] = {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
    png.insert(png.end(), signature, signature + sizeof(signature));
    std::vector<byte> ihdr;
    appendBe32(ihdr, width);
    appendBe32(ihdr, height);
    ihdr.push_back(8);
    ihdr.push_back(channels == 1 ? 0 : channels == 2 ? 4 : channels == 3 ? 2 : 6);
    ihdr.push_back(0);
    ihdr.push_back(0);
    ihdr.push_back(0);
    appendChunk(png, "IHDR", ihdr);
    appendChunk(png, "IDAT", zlib);
    appendChunk(png, "IEND", {});

    std::ofstream file(path, std::ios::binary);
    if (!file)
        return false;
    file.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    return file.good();
}

/// STB-based codec implementation for a single image format.
class StbCodec : public ICodec {
  public:
    /// Builds a codec for the given format and canonical lowercase extension.
    /// @param fmt Target image format.
    /// @param ext Canonical extension without the dot.
    StbCodec(ImageCodecFormat fmt, std::string ext) : info_{} {
        info_.format = fmt;
        info_.extension = std::move(ext);
        info_.readablePixels = {
            PixelFormat::Gray,
            PixelFormat::GrayAlpha,
            PixelFormat::RGB,
            PixelFormat::BGR,
            PixelFormat::RGBA,
            PixelFormat::BGRA};
        info_.writablePixels = {
            PixelFormat::Gray,
            PixelFormat::GrayAlpha,
            PixelFormat::RGB,
            PixelFormat::BGR,
            PixelFormat::RGBA,
            PixelFormat::BGRA};
    }

    /// Returns this codec's static capabilities.
    const CodecInfo& info() const override { return info_; }

    /// Sniffs magic bytes against this codec's file signature.
    /// @param data Pointer to the file's leading bytes.
    /// @param size Number of available bytes.
    /// @return True when the bytes match this format.
    bool canReadHeader(const byte* data, size_t size) const noexcept override {
        switch (info_.format) {
        case ImageCodecFormat::PNG:
            return size >= 8 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' &&
                   data[3] == 'G';
        case ImageCodecFormat::JPEG:
            return size >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF;
        case ImageCodecFormat::BMP:
            return size >= 2 && data[0] == 'B' && data[1] == 'M';
        default:
            return false;
        }
    }

    /// Decodes a file into `out`.
    /// @param out Image that receives the decoded pixels (fully replaced).
    /// @param path Path of the file to decode.
    /// @throws InvalidFileError When the file is unreadable or corrupt.
    /// @throws UnsupportedFormatError When channels cannot map to a pixel format.
    void read(Image& out, const std::string& path) override {
        int w = 0, h = 0, n = 0;
        unsigned char* px = stbi_load(path.c_str(), &w, &h, &n, 0);
        if (px == nullptr)
            throw InvalidFileError("StbCodec::read: " + path);

        const uint16 ch = static_cast<uint16>(n);
        const PixelFormat pf = pixelFormatForChannels(ch);
        try {
            if (pf == PixelFormat::Unknown) {
                throw UnsupportedFormatError("StbCodec::read: channel count " + std::to_string(n));
            }
            Image img(
                ImageFormat(pf, DataType::UInt8),
                static_cast<uint32>(w),
                static_cast<uint32>(h),
                pf == PixelFormat::Gray ? ColorSpace::Gray : ColorSpace::SRGB,
                (ch == 2 || ch == 4) ? AlphaMode::Straight : AlphaMode::None);
            const uint32 rowBytes = static_cast<uint32>(w) * ch;
            const uint32 height = static_cast<uint32>(h);
            parallelForRows(
                height,
                [&](size_t y) {
                    mem::copy(
                        img.row(static_cast<int32>(y)),
                        px + static_cast<uint64>(y) * rowBytes,
                        rowBytes);
                },
                ExecutionPolicy::parallel());
            out = std::move(img);
        } catch (...) {
            stbi_image_free(px);
            throw;
        }
        stbi_image_free(px);
    }

    /// Encodes `src` to `path` using the STB backend.
    /// @param src Image to encode.
    /// @param path Destination file path.
    /// @param opts Save options (e.g. JPEG quality).
    /// @throws InvalidParameterError When the image is empty.
    /// @throws UnsupportedFormatError When the source layout or format is unsupported.
    /// @throws CodecError When the underlying writer fails.
    void write(const Image& src, const std::string& path, const SaveOptions& opts) override {
        if (src.empty())
            throw InvalidParameterError("StbCodec::write: empty image");
        if (src.dataType() != DataType::UInt8) {
            throw UnsupportedFormatError("StbCodec::write: 8-bit only (STB backend)");
        }

        uint16 comp = 0;
        switch (info_.format) {
        case ImageCodecFormat::PNG:
            comp = src.channels();
            break;
        case ImageCodecFormat::JPEG:
            comp = (src.channels() == 1) ? 1 : 3;
            break;
        case ImageCodecFormat::BMP:
            comp = 3;
            break;
        case ImageCodecFormat::TGA:
            comp = (src.channels() == 1 || src.channels() == 3 || src.channels() == 4)
                       ? src.channels()
                       : 4;
            break;
        default:
            throw UnsupportedFormatError("StbCodec::write: unknown format");
        }

        uint64 packed = mem::mulChecked(
            static_cast<uint64>(comp),
            mem::mulChecked(src.width(), static_cast<uint64>(src.height())));
        mem::Buffer buf(static_cast<size_t>(packed));
        byte* dst = buf.data();
        const ConstImageView view = src.view();
        const uint32 iw = src.width(), ih = src.height();

        parallelForRows(
            ih,
            [&](size_t row) {
                const uint32 y = static_cast<uint32>(row);
                for (uint32 x = 0; x < iw; ++x) {
                    byte* o = dst + (static_cast<uint64>(y) * iw + x) * comp;
                    uint8 v[4];
                    uint8 a = 255;
                    switch (view.pixelFormat()) {
                    case PixelFormat::Gray:
                        v[0] = v[1] = v[2] = *view.sampleAt(x, y, 0);
                        break;
                    case PixelFormat::GrayAlpha:
                        v[0] = v[1] = v[2] = *view.sampleAt(x, y, 0);
                        a = *view.sampleAt(x, y, 1);
                        break;
                    case PixelFormat::RGB:
                        v[0] = *view.sampleAt(x, y, 0);
                        v[1] = *view.sampleAt(x, y, 1);
                        v[2] = *view.sampleAt(x, y, 2);
                        break;
                    case PixelFormat::BGR:
                        v[0] = *view.sampleAt(x, y, 2);
                        v[1] = *view.sampleAt(x, y, 1);
                        v[2] = *view.sampleAt(x, y, 0);
                        break;
                    case PixelFormat::RGBA:
                        v[0] = *view.sampleAt(x, y, 0);
                        v[1] = *view.sampleAt(x, y, 1);
                        v[2] = *view.sampleAt(x, y, 2);
                        a = *view.sampleAt(x, y, 3);
                        break;
                    case PixelFormat::BGRA:
                        v[0] = *view.sampleAt(x, y, 2);
                        v[1] = *view.sampleAt(x, y, 1);
                        v[2] = *view.sampleAt(x, y, 0);
                        a = *view.sampleAt(x, y, 3);
                        break;
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
                        o[0] = v[0];
                        o[1] = v[1];
                        o[2] = v[2];
                        break;
                    case 4:
                        o[0] = v[0];
                        o[1] = v[1];
                        o[2] = v[2];
                        o[3] = a;
                        break;
                    }
                }
            },
            ExecutionPolicy::parallel());

        int ok = 0;
        switch (info_.format) {
        case ImageCodecFormat::PNG:
            stbi_write_png_compression_level =
                std::min(9, std::max(0, static_cast<int>(opts.pngCompressionLevel)));
            if (stbi_write_png_compression_level == 0 &&
                static_cast<size_t>(src.width()) * comp + 1 <= 65535u) {
                ok = writeFastPng(path, src.width(), src.height(), comp, dst) ? 1 : 0;
            } else {
                ok = stbi_write_png(
                    path.c_str(),
                    static_cast<int>(src.width()),
                    static_cast<int>(src.height()),
                    comp,
                    dst,
                    0);
            }
            break;
        case ImageCodecFormat::JPEG:
            ok = stbi_write_jpg(
                path.c_str(),
                static_cast<int>(src.width()),
                static_cast<int>(src.height()),
                comp,
                dst,
                opts.jpegQuality > 0 ? opts.jpegQuality : kDefaultJpegQuality);
            break;
        case ImageCodecFormat::BMP:
            ok = stbi_write_bmp(
                path.c_str(),
                static_cast<int>(src.width()),
                static_cast<int>(src.height()),
                comp,
                dst);
            break;
        case ImageCodecFormat::TGA:
            ok = stbi_write_tga(
                path.c_str(),
                static_cast<int>(src.width()),
                static_cast<int>(src.height()),
                comp,
                dst);
            break;
        default:
            throw UnsupportedFormatError("StbCodec::write: unknown format");
        }
        if (!ok)
            throw CodecError("StbCodec::write: failed writing: " + path);
    }

  private:
    CodecInfo info_;
};

} // namespace

/// Registers the STB-backed codecs into the global registry (idempotent).
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