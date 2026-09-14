// examples/custom/custom_flip_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #5: Flip)
// ===================================================================
//
// What you learn here:
//   * Flipping re-orders pixels within a row: dst[x] = src[w - 1 - x].
//     Rows stay independent, so the whole image is trivially row-parallel.
//   * Contrast with rotate/crop: flip never moves data between rows, so it is
//     one of the cheapest operations there is (pure memory traffic).
//   * Note we create the destination explicitly sized/typed to match the
//     source. Keeping inputs read-only (const views) and the output separate
//     is the immutable/const-correct style OOP encourages.
//
// How it is OPTIMIZED:
//   * Row-level multithreading with parallelForRows + wantsParallel.
//   * mem::copy copies whole pixels byte-for-byte, so the compiler/library
//     picks wide (vector) copies instead of the naive 8-bit loop.
//
// Usage: custom_flip_demo <input> <output> [serial] [h|v]

#include "../example_util.h"

using namespace iml;

namespace {

inline void customFlipHorizontal(const ConstImageView& src, ImageView dst,
                                 const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customFlipHorizontal: invalid view");
    const uint32 w = src.width(), h = src.height(), ch = src.channels();
    const uint32 pxBytes = src.pixelSizeBytes();

    auto processRow = [&](uint32 y) {
        const byte* const sp = src.row(static_cast<int32>(y));
        byte* const dp = dst.row(static_cast<int32>(y));
        for (uint32 x = 0; x < w; ++x)
            mem::copy(dp + static_cast<uint64>(x) * pxBytes,
                      sp + static_cast<uint64>(w - 1 - x) * pxBytes, pxBytes);
    };

    if (!iml::detail::wantsParallel(policy, static_cast<size_t>(w) * h, 4096))
        for (uint32 y = 0; y < h; ++y) processRow(y);
    else
        parallelForRows(h, [&](size_t y) { processRow(static_cast<uint32>(y)); }, policy);
}

inline void customFlipVertical(const ConstImageView& src, ImageView dst,
                               const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customFlipVertical: invalid view");
    const uint32 w = src.width(), h = src.height();
    const uint32 ch = src.channels();
    const uint32 pxBytes = src.pixelSizeBytes();

    auto processRow = [&](uint32 y) {
        const byte* const sp = src.row(static_cast<int32>(h - 1 - y));
        byte* const dp = dst.row(static_cast<int32>(y));
        mem::copy(dp, sp, static_cast<size_t>(w) * pxBytes);
    };

    if (!iml::detail::wantsParallel(policy, static_cast<size_t>(w) * h, 4096))
        for (uint32 y = 0; y < h; ++y) processRow(y);
    else
        parallelForRows(h, [&](size_t y) { processRow(static_cast<uint32>(y)); }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: custom_flip_demo <input-image> <output-image> [serial] [h|v]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    const bool vertical = argc >= 5 && (argv[4][0] == 'v' || argv[4][0] == 'V');
    Image src = iml_example::loadOrDie(argv[1]);

    iml_example::banner("CUSTOM flip (kernel)", src);
    std::printf("  backend: %s | axis: %s\n",
                iml_example::policyName(serial), vertical ? "vertical" : "horizontal");

    Image out(src.format(), src.width(), src.height(), src.colorSpace());
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    if (vertical) {
        iml_example::timeAndReport("customFlipVertical", pixels, [&]() {
            customFlipVertical(src.view(), out.view(), policy);
        });
    } else {
        iml_example::timeAndReport("customFlipHorizontal", pixels, [&]() {
            customFlipHorizontal(src.view(), out.view(), policy);
        });
    }

    iml_example::saveOrDie(out, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}