// examples/custom/custom_oil_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #13: Oil paint)
// =========================================================================
//
// What you learn here:
//   * Oil paint is a STATISTICAL NEIGHBORHOOD OPERATOR: for each pixel, scan
//     a radius x radius window, build a histogram of quantized intensity
//     levels per channel, and output the most frequent level. That produces
//     the characteristic flat, "oily" regions.
//   * Unlike edge/blur (where every sample is weighted identically or by a
//     Gaussian), this filter uses a VOTE — the winner is the mode of the
//     histogram. The code is larger but the structure is identical to any
//     neighborhood filter: border sample → accumulate → write.
//   * The `intensityLevels` parameter (OOP: a tuning knob, not a global)
//     controls how many flat regions you get. Fewer levels = more posterized
//     regions = more oil-painting look.
//
// How it is OPTIMIZED:
//   * Histogram is a fixed-size stack array — no heap allocation.
//   * The outer pixel loop is clearly auto-vectorizable (reads are contiguous
//     when the window is on an interior row). The histogram is per-pixel and
//     tiny, so no false-sharing across threads.
//   * parallelForRows on the outer y-loop.
//
// Usage: custom_oil_demo <input> <output> [serial] [radius] [levels]

#include "../example_util.h"

using namespace iml;

namespace {

inline void customOilPaint(const ConstImageView& src, ImageView dst,
                           int radius, int intensityLevels,
                           const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customOilPaint: invalid view");
    if (radius < 1) throw InvalidParameterError("customOilPaint: radius < 1");

    const int32 w = static_cast<int32>(src.width());
    const int32 h = static_cast<int32>(src.height());
    const uint32 ch = std::min<uint32>(src.channels(), 4u);

    auto processRow = [&](int32 y) {
        for (int32 x = 0; x < w; ++x) {
            // Per-channel histogram of quantized levels: counts[chan][level]
            int counts[4][256];
            for (uint32 c = 0; c < ch; ++c) std::memset(counts[c], 0, sizeof counts[c]);

            // Scan the neighborhood.
            for (int32 dy = -radius; dy <= radius; ++dy) {
                const int32 sy = std::min(std::max(y + dy, 0), h - 1);
                for (int32 dx = -radius; dx <= radius; ++dx) {
                    const int32 sx = std::min(std::max(x + dx, 0), w - 1);
                    for (uint32 c = 0; c < ch; ++c) {
                        const float v = proc::detail::readNorm(src, sx, sy, c);
                        const int level = std::min(static_cast<int>(v * (intensityLevels - 1)),
                                                   intensityLevels - 1);
                        counts[c][level]++;
                    }
                }
            }

            // Per channel: find the level with max count.
            for (uint32 c = 0; c < ch; ++c) {
                int maxCount = -1, maxLevel = 0;
                for (int lv = 0; lv < intensityLevels; ++lv) {
                    if (counts[c][lv] > maxCount) {
                        maxCount = counts[c][lv];
                        maxLevel = lv;
                    }
                }
                const float out = static_cast<float>(maxLevel) / static_cast<float>(intensityLevels - 1);
                proc::detail::writeNorm(dst, x, y, c, out);
            }
        }
    };

    if (!iml::detail::wantsParallel(policy, static_cast<size_t>(w) * h, 4096))
        for (int32 y = 0; y < h; ++y) processRow(y);
    else
        parallelForRows(static_cast<size_t>(h),
                        [&](size_t y) { processRow(static_cast<int32>(y)); }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: custom_oil_demo <input-image> <output-image> [serial] [radius] [levels]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    const int radius  = argc >= 5 ? std::atoi(argv[4]) : 3;
    const int levels  = argc >= 6 ? std::atoi(argv[5]) : 20;
    Image src = iml_example::loadOrDie(argv[1]);

    iml_example::banner("CUSTOM oil paint (kernel)", src);
    std::printf("  backend: %s | radius: %d, levels: %d\n",
                iml_example::policyName(serial), radius, levels);

    Image out(src.format(), src.width(), src.height(), src.colorSpace());
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customOilPaint", pixels, [&]() {
        customOilPaint(src.view(), out.view(), radius, levels, policy);
    });

    iml_example::saveOrDie(out, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}