#pragma once
// imagelib/compression/PackedPixels.h
//
// Packed (sub-byte or packed-16) pixel formats and their conversions.
//
// Bit order inside a scanline: pixels are laid out LEFT-to-RIGHT,
// MOST-SIGNIFICANT-BIT-first within each byte (same convention as
// BitPacking.h). 8/16-bit formats are byte-aligned; 16-bit values are stored
// big-endian in the stream so the bit-oriented accessors stay uniform.
//
// Ownership: these functions never allocate; they operate on caller-owned
// row buffers.

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/compression/BitPacking.h"

namespace iml {
namespace comp {

// ---------------------------------------------------------------------------
// Packed pixel formats
// ---------------------------------------------------------------------------

enum class PackedFormat : uint16 {
    Gray1 = 0,   ///< 1-bit grayscale (monochrome)
    Gray2,       ///< 2-bit grayscale
    Gray4,       ///< 4-bit grayscale
    Gray8,       ///< 8-bit grayscale
    Gray16,      ///< 16-bit grayscale (big-endian in stream)
    RGB332,      ///< 8-bit RGB (3R, 3G, 2B)
    RGB565,      ///< 16-bit RGB (5R, 6G, 5B)
    BGR565,      ///< 16-bit BGR (5B, 6G, 5R)
    RGB666,      ///< 18-bit RGB (6R, 6G, 6B)
    RGBA4444,    ///< 16-bit RGBA (4+4+4+4)
    RGBA5551,    ///< 16-bit RGBA (5R,5G,5B,1A)
    Indexed1,    ///< 1-bit palette index
    Indexed2,    ///< 2-bit palette index
    Indexed4,    ///< 4-bit palette index
    Indexed8,    ///< 8-bit palette index
};

struct PackedFormatInfo {
    uint8  bitsPerPixel;
    bool   indexed;
    uint8  channels;      ///< semantic channels (color channels only for bpp counting)
    bool   hasAlpha;
};

/// Static descriptor for every PackedFormat.
constexpr PackedFormatInfo packedFormatInfo(PackedFormat f) noexcept {
    switch (f) {
        case PackedFormat::Gray1:   return {  1, false, 1, false };
        case PackedFormat::Gray2:   return {  2, false, 1, false };
        case PackedFormat::Gray4:   return {  4, false, 1, false };
        case PackedFormat::Gray8:   return {  8, false, 1, false };
        case PackedFormat::Gray16:  return { 16, false, 1, false };
        case PackedFormat::RGB332:  return {  8, false, 3, false };
        case PackedFormat::RGB565:  return { 16, false, 3, false };
        case PackedFormat::BGR565:  return { 16, false, 3, false };
        case PackedFormat::RGB666:  return { 18, false, 3, false };
        case PackedFormat::RGBA4444:return { 16, false, 4, true };
        case PackedFormat::RGBA5551:return { 16, false, 4, true };
        case PackedFormat::Indexed1:return {  1, true,  0, false };
        case PackedFormat::Indexed2:return {  2, true,  0, false };
        case PackedFormat::Indexed4:return {  4, true,  0, false };
        case PackedFormat::Indexed8:return {  8, true,  0, false };
    }
    return { 8, false, 1, false };
}

constexpr uint8 bitsPerPixel(PackedFormat f) noexcept { return packedFormatInfo(f).bitsPerPixel; }
constexpr bool   isIndexed(PackedFormat f) noexcept { return packedFormatInfo(f).indexed; }
constexpr uint8  bytesPerPixel(PackedFormat f) noexcept {
    return static_cast<uint8>(bytesForBits(bitsPerPixel(f)));
}

/// Rows are padded to `rowAlignment` bytes (default 4) for aligned access.
constexpr uint32 packedRowBytes(uint32 width, PackedFormat f,
                                uint32 rowAlignment = 4) noexcept {
    const uint64 bits = static_cast<uint64>(width) * bitsPerPixel(f);
    const uint64 bytes = bytesForBits(bits);
    if (rowAlignment <= 1) return static_cast<uint32>(bytes);
    return static_cast<uint32>(mem::alignUp(bytes, rowAlignment));
}

// ---------------------------------------------------------------------------
// MSB-first bit access into packed scanlines
// ---------------------------------------------------------------------------

/// Reads `nBits` (<= 32) starting at `bitPos` within a packed row.
inline uint64 readBitsRow(const byte* row, uint64 bitPos, uint32 nBits) noexcept {
    uint64 v = 0;
    for (uint32 i = 0; i < nBits; ++i) {
        const uint64 bpos   = bitPos + i;
        const byte   b      = row[bpos >> 3u];
        const uint32 shift  = 7u - static_cast<uint32>(bpos & 7u);
        v = (v << 1u) | ((b >> shift) & 1u);
    }
    return v;
}

/// Writes the low `nBits` of `value` at `bitPos` within a packed row.
inline void writeBitsRow(byte* row, uint64 bitPos, uint32 nBits, uint64 value) noexcept {
    for (uint32 i = 0; i < nBits; ++i) {
        const uint64 bpos   = bitPos + i;
        const uint32 shift  = 7u - static_cast<uint32>(bpos & 7u);
        const uint64 mask   = 1ull << (nBits - 1u - i);
        const byte   bit    = static_cast<byte>((value & mask) ? 1u : 0u);
        const uint32 idx    = static_cast<uint32>(bpos >> 3u);
        row[idx] = static_cast<byte>((row[idx] & ~(1u << shift))
                                     | (bit << shift));
    }
}

/// Reads an entire packed pixel at column `x` from a packed scanline.
inline uint64 readPackedPixel(const byte* row, uint32 x, PackedFormat f) noexcept {
    return readBitsRow(row, static_cast<uint64>(x) * bitsPerPixel(f), bitsPerPixel(f));
}

/// Writes a packed pixel at column `x` into a packed scanline.
inline void writePackedPixel(byte* row, uint32 x, PackedFormat f, uint64 value) noexcept {
    writeBitsRow(row, static_cast<uint64>(x) * bitsPerPixel(f), bitsPerPixel(f),
                 value & bitMask(bitsPerPixel(f)));
}

// ---------------------------------------------------------------------------
// Packed value <-> RGBA conversions (non-indexed)
// ---------------------------------------------------------------------------

/// Decomposes a packed value into canonical 8-bit RGBA. For indexed formats
/// returns the raw index. `mask` widths follow the format's channel count.
inline void unpackPixelRGBA(PackedFormat f, uint64 v,
                            uint8& r, uint8& g, uint8& b, uint8& a) noexcept {
    a = 255;
    switch (f) {
        case PackedFormat::Gray1:
        case PackedFormat::Gray2:
        case PackedFormat::Gray4:
        case PackedFormat::Gray8:
        case PackedFormat::Gray16: {
            const uint8 bpc = bitsPerPixel(f);
            const uint32 maxv = (1u << bpc) - 1u;
            const uint8  gray = static_cast<uint8>((v * 255u + maxv / 2u) / maxv);
            r = g = b = gray;
            break;
        }
        case PackedFormat::RGB332:
            r = static_cast<uint8>(((v >> 5u) & 7u) * 255u / 7u);
            g = static_cast<uint8>(((v >> 2u) & 7u) * 255u / 7u);
            b = static_cast<uint8>((v & 3u) * 255u / 3u);
            break;
        case PackedFormat::RGB565:
            r = static_cast<uint8>(((v >> 11u) & 31u) * 255u / 31u);
            g = static_cast<uint8>(((v >> 5u) & 63u) * 255u / 63u);
            b = static_cast<uint8>((v & 31u) * 255u / 31u);
            break;
        case PackedFormat::BGR565:
            b = static_cast<uint8>(((v >> 11u) & 31u) * 255u / 31u);
            g = static_cast<uint8>(((v >> 5u) & 63u) * 255u / 63u);
            r = static_cast<uint8>((v & 31u) * 255u / 31u);
            break;
        case PackedFormat::RGB666:
            r = static_cast<uint8>(((v >> 12u) & 63u) * 255u / 63u);
            g = static_cast<uint8>(((v >> 6u) & 63u) * 255u / 63u);
            b = static_cast<uint8>((v & 63u) * 255u / 63u);
            break;
        case PackedFormat::RGBA4444:
            r = static_cast<uint8>(((v >> 12u) & 15u) * 255u / 15u);
            g = static_cast<uint8>(((v >> 8u) & 15u) * 255u / 15u);
            b = static_cast<uint8>(((v >> 4u) & 15u) * 255u / 15u);
            a = static_cast<uint8>((v & 15u) * 255u / 15u);
            break;
        case PackedFormat::RGBA5551:
            r = static_cast<uint8>(((v >> 11u) & 31u) * 255u / 31u);
            g = static_cast<uint8>(((v >> 6u) & 31u) * 255u / 31u);
            b = static_cast<uint8>(((v >> 1u) & 31u) * 255u / 31u);
            a = (v & 1u) ? 255u : 0u;
            break;
        default: // indexed: return raw value in r
            r = static_cast<uint8>(v); g = b = 0; a = 0;
    }
}

/// Packs canonical 8-bit RGBA into `f` (indexed values returned verbatim via rPreset).
inline uint64 packPixelRGBA(PackedFormat f, uint8 r, uint8 g, uint8 b, uint8 a) noexcept {
    switch (f) {
        case PackedFormat::Gray1:
        case PackedFormat::Gray2:
        case PackedFormat::Gray4:
        case PackedFormat::Gray8:
        case PackedFormat::Gray16: {
            const uint8  bpc = bitsPerPixel(f);
            const uint32 maxv = (1u << bpc) - 1u;
            const uint32 gray = (77u * r + 150u * g + 29u * b) / 256u;
            return (static_cast<uint64>(gray) * maxv + 127u) / 255u;
        }
        case PackedFormat::RGB332:
            return (static_cast<uint64>(r >> 5u) << 5u)
                 | (static_cast<uint64>(g >> 5u) << 2u)
                 | static_cast<uint64>(b >> 6u);
        case PackedFormat::RGB565:
            return (static_cast<uint64>(r >> 3u) << 11u)
                 | (static_cast<uint64>(g >> 2u) << 5u)
                 | static_cast<uint64>(b >> 3u);
        case PackedFormat::BGR565:
            return (static_cast<uint64>(b >> 3u) << 11u)
                 | (static_cast<uint64>(g >> 2u) << 5u)
                 | static_cast<uint64>(r >> 3u);
        case PackedFormat::RGB666:
            return (static_cast<uint64>(r >> 2u) << 12u)
                 | (static_cast<uint64>(g >> 2u) << 6u)
                 | static_cast<uint64>(b >> 2u);
        case PackedFormat::RGBA4444:
            return (static_cast<uint64>(r >> 4u) << 12u)
                 | (static_cast<uint64>(g >> 4u) << 8u)
                 | (static_cast<uint64>(b >> 4u) << 4u)
                 | static_cast<uint64>(a >> 4u);
        case PackedFormat::RGBA5551:
            return (static_cast<uint64>(r >> 3u) << 11u)
                 | (static_cast<uint64>(g >> 3u) << 6u)
                 | (static_cast<uint64>(b >> 3u) << 1u)
                 | static_cast<uint64>(a >> 7u);
        default:
            return r; // raw index
    }
}

} // namespace comp
} // namespace iml