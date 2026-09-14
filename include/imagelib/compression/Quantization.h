#pragma once
// imagelib/compression/Quantization.h
//
// Color quantization: reduces the number of distinct channel values (uniform
// quantizer) optionally with dithering, and palette lookup. Operates on UInt8
// images. Whole-image entry points are implemented in Compression.cpp; the
// per-value helpers below are inline.
//
// Ownership: quantization never allocates through user code paths; the caller
// owns all buffers.

#include "imagelib/core/Types.h"
#include "imagelib/core/Image.h"
#include "imagelib/core/ImageView.h"
#include "imagelib/core/ExecutionPolicy.h"

namespace iml {
namespace comp {

enum class DitherMode : uint8 {
    None = 0,
    Ordered2x2,     ///< Bayer 2x2
    Ordered4x4,     ///< Bayer 4x4
    FloydSteinberg, ///< error diffusion
};

struct QuantizeParams {
    uint8  bitsR     = 5; ///< levels = (1 << bits) per RGB channel
    uint8  bitsG     = 6;
    uint8  bitsB     = 5;
    uint8  bitsGray  = 8; ///< used when quantizing single-channel images
    DitherMode dither = DitherMode::None;
};

/// Maps 0..255 to 0..(levels-1) (constant-ratio uniform quantizer).
constexpr uint8 quantizeU8(uint8 value, uint32 levels) noexcept {
    if (levels < 2) return 0;
    if (levels >= 256) return value;
    return static_cast<uint8>((static_cast<uint32>(value) * (levels - 1u)
                               + 127u) / 255u);
}

/// Maps 0..(levels-1) back to a representative 0..255 value.
constexpr uint8 dequantizeU8(uint8 index, uint32 levels) noexcept {
    if (levels < 2) return 128;
    if (levels >= 256) return index;
    return static_cast<uint8>((static_cast<uint32>(index) * 255u) / (levels - 1u));
}

/// Max EDT of a channel quantization (e.g. RGB332 loses up to <32 per channel).
constexpr uint32 maxQuantizationError(uint8 bits) noexcept {
    return bits >= 8 ? 0u : (255u / ((1u << bits) - 1u)) / 2u;
}

// ---------------------------------------------------------------------------
// Whole-image quantization (Compression.cpp)
// ---------------------------------------------------------------------------

/// Quantizes `src` into `dst` (both UInt8, same size). Overloads:
/// - single-channel images use bitsGray; multi-channel use bitsR/G/B per
///   channel; alpha is left untouched when present.
/// - `dst` must be pre-allocated by the caller with matching layout.
void quantizeImage(const ConstImageView& src, const ImageView& dst,
                   const QuantizeParams& params);

/// Policy-aware quantization: uniform/ordered dithering parallelizes across
/// rows; Floyd–Steinberg error diffusion stays single-threaded.
void quantizeImage(const ConstImageView& src, const ImageView& dst,
                   const QuantizeParams& params, const ExecutionPolicy& policy);

/// Convenience: returns a new quantized Image (deep copy of pixels).
Image quantize(const Image& src, const QuantizeParams& params);

// ---------------------------------------------------------------------------
// Palette helpers (Compression.cpp)
// ---------------------------------------------------------------------------

/// Common palette representation: N RGBA entries (4 bytes each).
using PaletteSpan = const byte*;

/// Builds a palette of `maxColors` entries for `src` using a simple
/// counting/histogram reduction (identical-quantizer seeding, RGB triplet
/// hashing). Result stored into caller-owned `outPalette` (maxColors*4 bytes);
/// returns the number of distinct entries actually used.
size_t buildPalette(const ConstImageView& src, uint32 maxColors,
                    byte* outPalette);

/// Finds the nearest palette index (RGB distance, alpha optional) for one pixel.
uint32 nearestPaletteIndex(const byte* palette, size_t count,
                           uint8 r, uint8 g, uint8 b, uint8 a,
                           bool accountAlpha);

/// Maps `src` (UInt8) to palette indices in `dst` (UInt8, single channel is
/// the destination layout) using nearestPaletteIndex.
void mapToPalette(const ConstImageView& src, const ImageView& dstIndex,
                  const byte* palette, size_t count);

} // namespace comp
} // namespace iml