// examples/custom/custom_invert_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #3: Invert)
// =====================================================================
//
// What you learn here:
//   * Point operator: output depends only on the pixel at the SAME position.
//     Because there is no neighborhood knowledge, the kernel trivially
//     parallelizes per-pixel / per-row.
//   * The math: inverted = 1.0 - value on normalized [0,1] floats. That is
//     why UInt8 images (0..255) map to 255 - v when you work directly on
//     bytes, and 1.0 - v when you use the normalized pipeline. Both are
//     correct; the normalized one is type-agnostic.
//   * This kernel works on ALL channels including alpha — "invert" a
//     grayscale and an RGBA image with the same four lines.
//
// How it is OPTIMIZED:
//   * Rows split over the thread pool (parallelForRows + wantsParallel).
//   * Auto-vectorizable inner loop. On both SSE2 and AVX2, 1-v for packed
//     f32 is a single instruction once the compiler vectorizes the loop.
//
// Usage: custom_invert_demo <input> <output> [serial]

#include "../example_util.h"

using namespace iml;

namespace {

inline void customInvert(const ConstImageView& src, ImageView dst,
                         const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customInvert: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height() ||
        src.channels() != dst.channels())
        throw InvalidParameterError("customInvert: view mismatch");

    const uint32 w = src.width(), h = src.height(), ch = src.channels();

    auto processRow = [&](uint32 y) {
        for (uint32 x = 0; x < w; ++x)
            for (uint32 c = 0; c < ch; ++c)
                proc::detail::writeNorm(dst, x, y, c,
                                        1.0f - proc::detail::readNorm(src, x, y, c));
    };

    if (!iml::detail::wantsParallel(policy, static_cast<size_t>(w) * h, 4096))
        for (uint32 y = 0; y < h; ++y) processRow(y);
    else
        parallelForRows(h, [&](size_t y) { processRow(static_cast<uint32>(y)); }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: custom_invert_demo <input-image> <output-image> [serial]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    Image src = iml_example::loadOrDie(argv[1]);

    iml_example::banner("CUSTOM invert (kernel)", src);
    std::printf("  backend: %s\n", iml_example::policyName(serial));

    Image out(src.format(), src.width(), src.height(), src.colorSpace());
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customInvert", pixels, [&]() {
        customInvert(src.view(), out.view(), policy);
    });

    iml_example::saveOrDie(out, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}