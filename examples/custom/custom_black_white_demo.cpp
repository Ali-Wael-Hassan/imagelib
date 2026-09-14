// examples/custom/custom_black_white_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #2: Black & White)
// ============================================================================
//
// What you learn here:
//   * A two-stage kernel: convert to luminance, then compare against a
//     threshold to produce a pure black or white pixel.
//   * The threshold is a "parameter" of the kernel. In OOP terms the kernel's
//     inputs (source, threshold) are passed explicitly instead of being hidden
//     global state. That makes the function predictable and testable.
//
// How it is OPTIMIZED:
//   * Multi-threaded with parallelForRows + wantsParallel (same pattern as
//     every kernel in this folder).
//   * The luminance vector is computed once per pixel, so there is no wasted
//     work; the compiler can keep r,g,b in vector registers.
//
// Usage: custom_black_white_demo <input> <output> [serial]

#include "../example_util.h"

using namespace iml;

namespace {

/// Luma -> binary: black when below `threshold` (normalized 0..1), else white.
inline float lumaToBinary(float r, float g, float b, float threshold) noexcept {
    const float l = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    return l < threshold ? 0.0f : 1.0f;
}

inline void customBlackWhite(const ConstImageView& src, ImageView dst,
                             float threshold, const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customBlackWhite: invalid view");
    if (threshold < 0.f || threshold > 1.f)
        throw InvalidParameterError("customBlackWhite: threshold out of [0,1]");

    const uint32 w = src.width(), h = src.height();
    const uint32 srcCh = src.channels();

    auto processRow = [&](uint32 y) {
        for (uint32 x = 0; x < w; ++x) {
            float r, g, b;
            if (srcCh >= 3) {
                r = proc::detail::readNorm(src, x, y, 0);
                g = proc::detail::readNorm(src, x, y, 1);
                b = proc::detail::readNorm(src, x, y, 2);
            } else {
                r = g = b = proc::detail::readNorm(src, x, y, 0);
            }
            proc::detail::writeNorm(dst, x, y, 0, lumaToBinary(r, g, b, threshold));
        }
    };

    if (!iml::detail::wantsParallel(policy, static_cast<size_t>(w) * h, 4096))
        for (uint32 y = 0; y < h; ++y) processRow(y);
    else
        parallelForRows(h, [&](size_t y) { processRow(static_cast<uint32>(y)); }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: custom_black_white_demo <input-image> <output-image> [serial] [threshold]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    const float threshold = argc >= 5 ? static_cast<float>(std::atof(argv[4])) : 0.5f;
    Image src = iml_example::loadOrDie(argv[1]);

    iml_example::banner("CUSTOM black & white (kernel)", src);
    std::printf("  backend: %s | threshold: %.2f\n",
                iml_example::policyName(serial), threshold);

    Image out(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customBlackWhite", pixels, [&]() {
        customBlackWhite(src.view(), out.view(), threshold, policy);
    });

    iml_example::saveOrDie(out, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}