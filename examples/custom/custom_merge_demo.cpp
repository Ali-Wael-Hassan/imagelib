// examples/custom/custom_merge_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #4: Merge)
// ====================================================================
//
// What you learn here:
//   * Merge takes TWO inputs (base + overlay) and one output. OOP-wise that is
//     an operation with multiple parameters — still just a well-typed function.
//   * The blend amount is controlled by the overlay's alpha channel times a
//     user `opacity`. If the overlay has no alpha, it is treated as opaque.
//   * When the source images are plain 8-bit data we can avoid the float
//     conversion entirely and operate directly on BYTES — a "packed" kernel.
//
// How it is OPTIMIZED (two levels, pick the right one for real projects):
//   1. Normalized path (any data type): parallelForRows + readNorm/writeNorm.
//   2. Packed 8-bit path (UInt8, tightly packed rows, 50% blend):
//      simd::averageU8 processes many pixels per batch. We resolve the SIMD
//      tier with simd::dispatchMode(policy, n), split the row into vector+
//      scalar spans with simd::split(), and process whole vectors. See
//      imagelib/simd/Simd.h — ships SSE2/NEON implementations and a scalar
//      fallback, so this compiles and runs everywhere.
//
// Usage: custom_merge_demo <base-image> <overlay-image> <output> [serial] [opacity]

#include "../example_util.h"

using namespace iml;

namespace {

/// General normalized blend of one pixel. Returns the blended color channels.
inline void blendPixelNorm(const ConstImageView& base, const ConstImageView& ov,
                           uint32 x, uint32 y, uint32 c, float blend,
                           float& out) {
    const float bg = proc::detail::readNorm(base, x, y, c);
    const float fg = proc::detail::readNorm(ov, x, y, c);
    out = bg * (1.0f - blend) + fg * blend;
}

inline void customMerge(const ConstImageView& base, const ConstImageView& ov,
                        ImageView dst, float opacity, const ExecutionPolicy& policy) {
    if (!base.valid() || !ov.valid() || !dst.valid())
        throw InvalidParameterError("customMerge: invalid view");
    if (base.width() != ov.width() || base.height() != ov.height() ||
        base.width() != dst.width() || base.height() != dst.height())
        throw InvalidParameterError("customMerge: dimension mismatch");
    if (opacity < 0.f || opacity > 1.f)
        throw InvalidParameterError("customMerge: opacity out of [0,1]");

    const uint32 w = base.width(), h = base.height(), ch = base.channels();
    const bool ovHasAlpha = ov.channels() >= 4;
    const size_t pixels = static_cast<size_t>(w) * h;

    // Packed fast path: 8-bit data, tightly packed rows, no alpha weighting,
    // exactly 50% blend => out = (a + b) / 2 = simd::averageU8.
    const bool packed = dst.dataType() == DataType::UInt8
                     && base.dataType() == DataType::UInt8
                     && ov.dataType() == DataType::UInt8
                     && base.pixelStride() == base.channels()
                     && ov.pixelStride() == ov.channels()
                     && dst.pixelStride() == dst.channels()
                     && !ovHasAlpha
                     && opacity == 0.5f;

    if (packed) {
        parallelForRows(h, [&](size_t y) {
            for (uint32 c = 0; c < ch; ++c) {
                const byte* a = base.row(static_cast<int32>(y)) + c * sizeof(uint8);
                const byte* b = ov.row(static_cast<int32>(y)) + c * sizeof(uint8);
                byte* d = dst.row(static_cast<int32>(y)) + c * sizeof(uint8);
                if (simd::dispatchMode(policy, pixels) == simd::DispatchMode::Simd) {
                    const simd::SimdSplit sp = simd::split<uint8>(w, a);
                    // non-aligned head, then full vectors, then scalar tail
                    for (size_t i = 0; i < sp.headBlocks; ++i) d[i] = static_cast<uint8>((a[i] + b[i]) >> 1);
                    simd::averageU8(d + sp.headBlocks, a + sp.headBlocks,
                                    b + sp.headBlocks, sp.vectorElems);
                    const size_t t0 = sp.headBlocks + sp.vectorElems;
                    for (size_t i = 0; i < sp.tail; ++i) d[t0 + i] = static_cast<uint8>((a[t0 + i] + b[t0 + i]) >> 1);
                } else {
                    for (uint32 x = 0; x < w; ++x)
                        d[x] = static_cast<uint8>((a[x] + b[x]) >> 1);
                }
            }
        }, policy);
        return;
    }

    // General normalized path (any data type / blend factor).
    auto processRow = [&](uint32 y) {
        for (uint32 x = 0; x < w; ++x) {
            float alpha = ovHasAlpha ? proc::detail::readNorm(ov, x, y, 3) : 1.0f;
            const float blend = opacity * alpha;
            if (blend <= 0.f) { // shortcut: keep the base pixel untouched
                for (uint32 c = 0; c < ch; ++c)
                    proc::detail::writeNorm(dst, x, y, c, proc::detail::readNorm(base, x, y, c));
                continue;
            }
            for (uint32 c = 0; c < ch; ++c) {
                float v;
                blendPixelNorm(base, ov, x, y, c, blend, v);
                proc::detail::writeNorm(dst, x, y, c, v);
            }
        }
    };

    if (!iml::detail::wantsParallel(policy, pixels, 4096))
        for (uint32 y = 0; y < h; ++y) processRow(y);
    else
        parallelForRows(h, [&](size_t y) { processRow(static_cast<uint32>(y)); }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::printf("Usage: custom_merge_demo <base> <overlay> <output> [serial] [opacity]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    const float opacity = argc >= 6 ? static_cast<float>(std::atof(argv[5])) : 0.5f;
    Image base = iml_example::loadOrDie(argv[1]);
    Image ov   = iml_example::loadOrDie(argv[2]);
    if (base.width() != ov.width() || base.height() != ov.height()) {
        std::printf("error: images differ in size (%ux%u vs %ux%u)\n",
                    base.width(), base.height(), ov.width(), ov.height());
        return 1;
    }

    iml_example::banner("CUSTOM merge (kernel)", base);
    std::printf("  backend: %s | opacity: %.2f | packed8-bit: %s\n",
                iml_example::policyName(serial), opacity,
                (opacity == 0.5f && ov.channels() < 4) ? "SIMD averageU8" : "no");

    Image out(base.format(), base.width(), base.height(), base.colorSpace());
    const uint64 pixels = static_cast<uint64>(base.width()) * base.height();
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customMerge", pixels, [&]() {
        customMerge(base.view(), ov.view(), out.view(), opacity, policy);
    });

    iml_example::saveOrDie(out, argv[3]);
    std::printf("  wrote %s\n", argv[3]);
    return 0;
}