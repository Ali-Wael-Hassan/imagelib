// imagelib/src/compression/Compression.cpp
//
// Whole-image compression algorithms: uniform quantization with dithering,
// palette building/lookup, and PackedImage pack/unpacking.

#include "imagelib/compression/Quantization.h"
#include "imagelib/compression/PackedPixels.h"
#include "imagelib/compression/PackedImage.h"
#include "imagelib/core/memory/Memory.h"
#include "imagelib/core/ExecutionPolicy.h"
#include "imagelib/threading/ParallelFor.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <vector>

namespace iml {
namespace comp {

namespace {

PixelFormat pixelFormatForChannels(uint16 ch) {
    switch (ch) {
        case 1:  return PixelFormat::Gray;
        case 2:  return PixelFormat::GrayAlpha;
        case 3:  return PixelFormat::RGB;
        case 4:  return PixelFormat::RGBA;
        default: return PixelFormat::Unknown;
    }
}

constexpr uint8 clampU8(int32 v) noexcept {
    return static_cast<uint8>(v < 0 ? 0 : (v > 255 ? 255 : v));
}

uint32 levelsForChannel(const QuantizeParams& p, uint16 channel, uint16 total) noexcept {
    if (total == 1) return 1u << (p.bitsGray > 8 ? 8 : p.bitsGray);
    if (channel == 0) return 1u << (p.bitsR > 8 ? 8 : p.bitsR);
    if (channel == 1) return 1u << (p.bitsG > 8 ? 8 : p.bitsG);
    if (channel == 2) return 1u << (p.bitsB > 8 ? 8 : p.bitsB);
    return 256u; // alpha: no quantization
}

uint8 bayer2x2(uint32 x, uint32 y) noexcept {
    static const uint8 m[4] = {0, 2, 3, 1};
    return m[(y & 1u) * 2u + (x & 1u)];
}
uint8 bayer4x4(uint32 x, uint32 y) noexcept {
    static const uint8 m[16] = {
         0,  8,  2, 10,
        12,  4, 14,  6,
         3, 11,  1,  9,
        15,  7, 13,  5,
    };
    return m[(y & 3u) * 4u + (x & 3u)];
}

int32 orderedBias(DitherMode mode, uint32 x, uint32 y, uint32 maxErr) noexcept {
    if (mode == DitherMode::Ordered2x2) {
        const int32 v = bayer2x2(x, y);
        return (v * static_cast<int32>(maxErr) * 2) / 3 - static_cast<int32>(maxErr);
    }
    if (mode == DitherMode::Ordered4x4) {
        const int32 v = bayer4x4(x, y);
        return (v * static_cast<int32>(maxErr) * 2) / 15 - static_cast<int32>(maxErr);
    }
    return 0;
}

/// Reads a pixel's canonical RGBA honoring the source PixelFormat.
void readCanonicalRGBA(const ConstImageView& view, uint32 x, uint32 y,
                       uint8& r, uint8& g, uint8& b, uint8& a) noexcept {
    a = 255;
    const byte* p = view.pixelAt(x, y);
    switch (view.pixelFormat()) {
        case PixelFormat::Gray:
            r = g = b = *p; break;
        case PixelFormat::GrayAlpha:
            r = g = b = p[0]; a = p[1]; break;
        case PixelFormat::RGB:
            r = p[0]; g = p[1]; b = p[2]; break;
        case PixelFormat::BGR:
            r = p[2]; g = p[1]; b = p[0]; break;
        case PixelFormat::RGBA:
            r = p[0]; g = p[1]; b = p[2]; a = p[3]; break;
        case PixelFormat::BGRA:
            r = p[2]; g = p[1]; b = p[0]; a = p[3]; break;
        default:
            r = g = b = 0; a = 0;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Uniform quantization
// ---------------------------------------------------------------------------

namespace {

/// Uniform/ordered quantization: every pixel is independent, so rows are
/// processed over the thread pool when `policy` asks for it.
inline void quantizeNonFs(const ConstImageView& src, const ImageView& dst,
                          const QuantizeParams& params,
                          const ExecutionPolicy& policy) {
    const uint32 w = src.width(), h = src.height();
    const uint32 ch = src.channels();
    if (!detail::wantsParallel(policy, static_cast<size_t>(w) * h * ch, 4096)) {
        for (uint32 y = 0; y < h; ++y) {
            for (uint32 x = 0; x < w; ++x) {
                for (uint32 c = 0; c < ch; ++c) {
                    const uint32 lvl = levelsForChannel(params, static_cast<uint16>(c),
                                                        static_cast<uint16>(ch));
                    const uint8 raw = *src.sampleAt(x, y, c);
                    if (lvl >= 256) { *dst.sampleAt(x, y, c) = raw; continue; }
                    int32 v = raw;
                    if (params.dither != DitherMode::None) {
                        v += orderedBias(params.dither, x, y, maxQuantizationError(static_cast<uint8>(lvl)));
                    }
                    const uint8 idx = quantizeU8(clampU8(v), lvl);
                    *dst.sampleAt(x, y, c) = dequantizeU8(idx, lvl);
                }
            }
        }
        return;
    }
    const uint32 W = w, H = h, CH = ch;
    parallelForRows(H, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < W; ++x) {
            for (uint32 c = 0; c < CH; ++c) {
                const uint32 lvl = levelsForChannel(params, static_cast<uint16>(c),
                                                    static_cast<uint16>(CH));
                const uint8 raw = *src.sampleAt(x, y, c);
                if (lvl >= 256) { *dst.sampleAt(x, y, c) = raw; continue; }
                int32 v = raw;
                if (params.dither != DitherMode::None) {
                    v += orderedBias(params.dither, x, y, maxQuantizationError(static_cast<uint8>(lvl)));
                }
                const uint8 idx = quantizeU8(clampU8(v), lvl);
                *dst.sampleAt(x, y, c) = dequantizeU8(idx, lvl);
            }
        }
    }, policy);
}

/// Floyd–Steinberg diffusion is sequential by construction (each pixel consumes
/// error left behind by its left/upper neighbours), so it always runs serially.
inline void quantizeFs(const ConstImageView& src, const ImageView& dst,
                       const QuantizeParams& params) {
    const uint32 w = src.width(), h = src.height();
    const uint32 ch = src.channels();
    const bool  doAlpha = (ch == 4);
    const uint32 nProc   = doAlpha ? 3u : ch;
    std::vector<int32> cur(nProc * (w + 2), 0);
    std::vector<int32> prev(nProc * (w + 2), 0);
    auto at = [&](std::vector<int32>& row, uint32 c, uint32 x) -> int32& {
        return row[c * (w + 2u) + x + 1u];
    };

    for (uint32 y = 0; y < h; ++y) {
        std::fill(cur.begin(), cur.end(), 0);
        for (uint32 x = 0; x < w; ++x) {
            for (uint32 c = 0; c < nProc; ++c) {
                const uint32 lvl = levelsForChannel(params, static_cast<uint16>(c),
                                                    static_cast<uint16>(ch));
                const int32 raw = *src.sampleAt(x, y, c);
                if (lvl >= 256) {
                    *dst.sampleAt(x, y, c) = clampU8(raw);
                    continue;
                }
                int32 actual = raw + at(cur, c, x);
                if (actual < 0) actual = 0;
                if (actual > 255) actual = 255;
                const uint8 idx = quantizeU8(static_cast<uint8>(actual), lvl);
                const uint8 repr = dequantizeU8(idx, lvl);
                *dst.sampleAt(x, y, c) = repr;
                const int32 e = actual - repr;

                at(cur, c, x + 1) += (e * 7) / 16;
                at(prev, c, x - 1) += (e * 3) / 16;
                at(prev, c, x)     += (e * 5) / 16;
                at(prev, c, x + 1) += (e * 1) / 16;
            }
            if (doAlpha) *dst.sampleAt(x, y, 3) = *src.sampleAt(x, y, 3);
        }
        std::swap(cur, prev);
    }
}

} // namespace

void quantizeImage(const ConstImageView& src, const ImageView& dst,
                   const QuantizeParams& params) {
    if (!src.valid() || !dst.valid()) {
        throw InvalidParameterError("quantizeImage: invalid view");
    }
    if (src.dataType() != DataType::UInt8 || dst.dataType() != DataType::UInt8) {
        throw UnsupportedFormatError("quantizeImage: UInt8 only");
    }
    if (src.width() != dst.width() || src.height() != dst.height()
        || src.channels() != dst.channels()) {
        throw InvalidDimensionError("quantizeImage: src/dst layout mismatch");
    }

    if (params.dither != DitherMode::FloydSteinberg) {
        quantizeNonFs(src, dst, params, ExecutionPolicy());
        return;
    }
    quantizeFs(src, dst, params);
}

/// Policy-aware quantization. Floyd–Steinberg remains serial (sequentially
/// dependent pixels); uniform/ordered dithering parallelizes across rows.
void quantizeImage(const ConstImageView& src, const ImageView& dst,
                   const QuantizeParams& params, const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid()) {
        throw InvalidParameterError("quantizeImage: invalid view");
    }
    if (src.dataType() != DataType::UInt8 || dst.dataType() != DataType::UInt8) {
        throw UnsupportedFormatError("quantizeImage: UInt8 only");
    }
    if (src.width() != dst.width() || src.height() != dst.height()
        || src.channels() != dst.channels()) {
        throw InvalidDimensionError("quantizeImage: src/dst layout mismatch");
    }

    if (params.dither != DitherMode::FloydSteinberg) {
        quantizeNonFs(src, dst, params, policy);
        return;
    }
    quantizeFs(src, dst, params);
}

Image quantize(const Image& src, const QuantizeParams& params) {
    if (src.empty()) return Image();
    Image out(src.format(), src.width(), src.height(), src.colorSpace(), src.alphaMode());
    quantizeImage(src.view(), out.view(), params);
    return out;
}

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------

size_t buildPalette(const ConstImageView& src, uint32 maxColors, byte* outPalette) {
    if (!src.valid() || maxColors == 0 || outPalette == nullptr) {
        throw InvalidParameterError("buildPalette: bad arguments");
    }
    if (maxColors > 256) maxColors = 256;

    // 8^3 histogram over the top 3 bits of each RGB channel.
    constexpr size_t kBin = 512;
    uint32 hist[kBin] = {};
    for (uint32 y = 0; y < src.height(); ++y) {
        for (uint32 x = 0; x < src.width(); ++x) {
            uint8 r, g, b, a;
            readCanonicalRGBA(src, x, y, r, g, b, a);
            const size_t bin = (static_cast<size_t>(r) >> 5u) << 6u
                             | (static_cast<size_t>(g) >> 5u) << 3u
                             | (static_cast<size_t>(b) >> 5u);
            if (hist[bin] != ~0u) ++hist[bin];
        }
    }

    // Select the most frequent bins up to maxColors.
    struct Cand { size_t bin; uint32 count; };
    std::vector<Cand> cands;
    cands.reserve(kBin);
    for (size_t i = 0; i < kBin; ++i) {
        if (hist[i]) cands.push_back({ i, hist[i] });
    }
    std::sort(cands.begin(), cands.end(),
              [](const Cand& x, const Cand& y) { return x.count > y.count; });
    if (cands.size() > maxColors) cands.resize(maxColors);

    for (size_t i = 0; i < cands.size(); ++i) {
        const byte cc = static_cast<byte>((cands[i].bin & 7u) * 255u / 7u);
        const byte cg = static_cast<byte>(((cands[i].bin >> 3u) & 7u) * 255u / 7u);
        const byte cr = static_cast<byte>(((cands[i].bin >> 6u) & 7u) * 255u / 7u);
        outPalette[i * 4 + 0] = cr;
        outPalette[i * 4 + 1] = cg;
        outPalette[i * 4 + 2] = cc;
        outPalette[i * 4 + 3] = 255;
    }
    return cands.size();
}

uint32 nearestPaletteIndex(const byte* palette, size_t count,
                           uint8 r, uint8 g, uint8 b, uint8 a,
                           bool accountAlpha) {
    if (count == 0) throw InvalidParameterError("nearestPaletteIndex: empty palette");
    int32 bestDist = INT32_MAX;
    uint32 best = 0;
    for (size_t i = 0; i < count; ++i) {
        const int32 dr = static_cast<int32>(palette[i * 4 + 0]) - r;
        const int32 dg = static_cast<int32>(palette[i * 4 + 1]) - g;
        const int32 db = static_cast<int32>(palette[i * 4 + 2]) - b;
        int32 d = dr * dr + dg * dg + db * db;
        if (accountAlpha) {
            const int32 da = static_cast<int32>(palette[i * 4 + 3]) - a;
            d += da * da;
        }
        if (d < bestDist) { bestDist = d; best = static_cast<uint32>(i); }
    }
    return best;
}

void mapToPalette(const ConstImageView& src, const ImageView& dstIndex,
                  const byte* palette, size_t count) {
    if (!src.valid() || !dstIndex.valid()) {
        throw InvalidParameterError("mapToPalette: invalid view");
    }
    if (src.width() != dstIndex.width() || src.height() != dstIndex.height()
        || palette == nullptr || count == 0) {
        throw InvalidParameterError("mapToPalette: bad arguments");
    }
    for (uint32 y = 0; y < src.height(); ++y) {
        for (uint32 x = 0; x < src.width(); ++x) {
            uint8 r, g, b, a;
            readCanonicalRGBA(src, x, y, r, g, b, a);
            *dstIndex.sampleAt(x, y, 0) = static_cast<byte>(
                nearestPaletteIndex(palette, count, r, g, b, a, src.channels() == 4));
        }
    }
}

// ---------------------------------------------------------------------------
// PackedImage
// ---------------------------------------------------------------------------

PackedImage::PackedImage(PackedFormat fmt, uint32 w, uint32 h, ColorSpace cs,
                         uint32 rowAlignment)
    : rowAlign_(rowAlignment) {
    allocate(fmt, w, h, cs, rowAlignment);
}

PackedImage::PackedImage(const Image& src, PackedFormat fmt,
                         const byte* palette, size_t paletteCount,
                         uint32 rowAlignment)
    : rowAlign_(rowAlignment) {
    allocate(fmt, src.width(), src.height(), src.colorSpace(), rowAlignment);
    pack(src, palette, paletteCount);
}

void PackedImage::allocate(PackedFormat fmt, uint32 w, uint32 h, ColorSpace cs,
                           uint32 rowAlignment) {
    if (w == 0 || h == 0) throw InvalidDimensionError("PackedImage: zero dimension");
    if (rowAlignment == 0) throw InvalidParameterError("PackedImage: bad row alignment");

    release();
    format_ = fmt;
    width_ = w;
    height_ = h;
    rowAlign_ = rowAlignment;
    rowBytes_ = packedRowBytes(w, fmt, rowAlignment);
    colorSpace_ = cs;

    uint64 bytes = mem::mulChecked(static_cast<uint64>(rowBytes_), static_cast<uint64>(h));
    pixels_ = mem::Buffer(static_cast<size_t>(bytes), rowAlignment);
}

void PackedImage::release() noexcept {
    format_ = PackedFormat::Gray8;
    width_ = height_ = rowBytes_ = 0;
    colorSpace_ = ColorSpace::Unknown;
    pixels_ = mem::Buffer();
    palette_ = mem::Buffer();
    paletteCount_ = 0;
}

void PackedImage::swap(PackedImage& other) noexcept {
    std::swap(format_, other.format_);
    std::swap(width_, other.width_);
    std::swap(height_, other.height_);
    std::swap(rowBytes_, other.rowBytes_);
    std::swap(rowAlign_, other.rowAlign_);
    std::swap(colorSpace_, other.colorSpace_);
    pixels_.swap(other.pixels_);
    palette_.swap(other.palette_);
    std::swap(paletteCount_, other.paletteCount_);
}

PackedImage::PackedImage(const PackedImage& other)
    : format_(other.format_), width_(other.width_), height_(other.height_),
      rowBytes_(other.rowBytes_), rowAlign_(other.rowAlign_),
      colorSpace_(other.colorSpace_),
      pixels_(other.pixels_), palette_(other.palette_),
      paletteCount_(other.paletteCount_) {}

PackedImage& PackedImage::operator=(const PackedImage& other) {
    if (this == &other) return *this;
    format_ = other.format_;
    width_ = other.width_;
    height_ = other.height_;
    rowBytes_ = other.rowBytes_;
    rowAlign_ = other.rowAlign_;
    colorSpace_ = other.colorSpace_;
    pixels_ = other.pixels_;
    palette_ = other.palette_;
    paletteCount_ = other.paletteCount_;
    return *this;
}

PackedImage PackedImage::clone() const { return *this; }

void PackedImage::setPalette(const byte* rgba, size_t count) {
    if (count > 256) throw InvalidParameterError("PackedImage: palette too large");
    palette_ = mem::Buffer(count * 4);
    std::memcpy(palette_.data(), rgba, count * 4);
    paletteCount_ = count;
}

void PackedImage::pack(const Image& src, const byte* palette, size_t paletteCount) {
    if (src.empty()) throw InvalidParameterError("PackedImage::pack: empty source");
    if (src.dataType() != DataType::UInt8) {
        throw UnsupportedFormatError("PackedImage::pack: UInt8 source required");
    }
    if (src.width() != width_ || src.height() != height_) {
        throw InvalidDimensionError("PackedImage::pack: size mismatch");
    }

    const ConstImageView view = src.view();
    const bool indexed = isIndexed(format_);
    if (indexed && (palette == nullptr || paletteCount == 0)) {
        throw InvalidParameterError("PackedImage::pack: indexed format requires a palette");
    }
    if (indexed) setPalette(palette, paletteCount);
    else { palette_ = mem::Buffer(); paletteCount_ = 0; }

    for (uint32 y = 0; y < height_; ++y) {
        byte* row = pixels_.data() + static_cast<uint64>(y) * rowBytes_;
        for (uint32 x = 0; x < width_; ++x) {
            uint64 value;
            if (indexed) {
                uint8 r, g, b, a;
                readCanonicalRGBA(view, x, y, r, g, b, a);
                value = nearestPaletteIndex(palette_.data(), paletteCount_,
                                            r, g, b, a, src.channels() == 4);
            } else {
                uint8 r, g, b, a;
                readCanonicalRGBA(view, x, y, r, g, b, a);
                value = packPixelRGBA(format_, r, g, b, a);
            }
            writePackedPixel(row, x, format_, value);
        }
    }
}

void PackedImage::unpackTo(Image& out, ColorSpace csOverride) const {
    if (empty()) { out = Image(); return; }

    const bool indexed = isIndexed(format_);
    const uint16 ch = indexed ? 4u : static_cast<uint16>(
        packedFormatInfo(format_).channels);

    ColorSpace cs = (csOverride != ColorSpace::Unknown) ? csOverride : colorSpace_;
    if (cs == ColorSpace::Unknown) {
        cs = (ch == 1) ? ColorSpace::Gray : ColorSpace::SRGB;
    }
    Image tmp(ImageFormat(pixelFormatForChannels(ch), DataType::UInt8),
              width_, height_, cs, ch == 4 ? AlphaMode::Straight : AlphaMode::None);

    for (uint32 y = 0; y < height_; ++y) {
        const byte* prow = pixels_.data() + static_cast<uint64>(y) * rowBytes_;
        for (uint32 x = 0; x < width_; ++x) {
            uint8 r, g, b, a;
            if (indexed) {
                const uint64 idx = readPackedPixel(prow, x, format_);
                if (idx >= paletteCount_) throw InvalidParameterError("PackedImage: index out of palette range");
                const byte* pe = palette_.data() + idx * 4;
                r = pe[0]; g = pe[1]; b = pe[2]; a = pe[3];
            } else {
                unpackPixelRGBA(format_, readPackedPixel(prow, x, format_), r, g, b, a);
            }
            byte* px = tmp.pixel(x, y);
            if (ch == 4) {
                px[0] = r; px[1] = g; px[2] = b; px[3] = a;
            } else if (ch == 3) {
                px[0] = r; px[1] = g; px[2] = b;
            } else {
                px[0] = static_cast<byte>((77 * r + 150 * g + 29 * b) / 256);
            }
        }
    }
    out = std::move(tmp);
}

Image PackedImage::unpack(ColorSpace cs) const {
    Image out;
    unpackTo(out, cs);
    return out;
}

} // namespace comp
} // namespace iml