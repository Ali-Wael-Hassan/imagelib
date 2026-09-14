// examples/custom/custom_grayscale_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #1: Grayscale)
// ========================================================================
//
// What you learn here:
//   * A filter is just a function: it reads normalized pixels from a source
//     view and writes normalized pixels into a destination view.
//   * "Normalized" means every channel is a float in [0,1] regardless of the
//     image's storage type (UInt8/UInt16/Float32). You read with
//     proc::detail::readNorm and you write with proc::detail::writeNorm,
//     so ONE kernel works for every data type.
//   * Encapsulation (OOP): the kernel hides the algorithm behind a clean
//     function signature. Callers only pass views + an ExecutionPolicy.
//
// How it is OPTIMIZED:
//   * Multi-threaded: rows are independent, so we split them across the
//     thread pool with parallelForRows. Small images fall back to serial.
//   * SIMD-friendly: the inner loop is a straight-line float computation the
//     compiler auto-vectorizes (SSE2 is baseline on x86-64; build with
//     -DIML_ENABLE_NATIVE=ON to unlock AVX2/AVX-512 on your CPU).
//
// Usage: custom_grayscale_demo <input> <output> [serial]

#include "../example_util.h"

using namespace iml;

namespace {

/// Rec.709 luminance of one pixel: the standard "grayscale" weighting.
/// This is the kernel: the same math applies to every pixel, every row.
inline float luminanceN(float r, float g, float b) noexcept {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

/// Custom grayscale conversion: dst[y][x] = luminance(src[y][x]).
///
/// OOP note: the destination view is passed by value (it is a lightweight
/// handle to caller-owned memory), so the kernel never owns or frees the
/// Image. That separation of ownership is a core OOP principle.
inline void customToGray(const ConstImageView& src, ImageView dst, const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customToGray: invalid view");
    if (src.width() != dst.width() || src.height() != dst.height())
        throw InvalidParameterError("customToGray: dimension mismatch");

    const uint32 w = src.width(), h = src.height();
    const uint32 srcCh = src.channels();

    // The per-row body lives in its own function: clean, reusable, testable.
    auto processRow = [&](uint32 y) {
        for (uint32 x = 0; x < w; ++x) {
            float r, g, b;
            if (srcCh >= 3) {
                r = proc::detail::readNorm(src, x, y, 0);
                g = proc::detail::readNorm(src, x, y, 1);
                b = proc::detail::readNorm(src, x, y, 2);
            } else { // already gray-ish (1 or 2 channels)
                r = g = b = proc::detail::readNorm(src, x, y, 0);
            }
            proc::detail::writeNorm(dst, x, y, 0, luminanceN(r, g, b));
        }
    };

    // Multi-threading: split rows over the pool only when it is worth it.
    // wantsParallel says "yes" for Parallel/SimdParallel, "no" for Serial,
    // and for Auto it compares the pixel count to the given threshold.
    if (!iml::detail::wantsParallel(policy, static_cast<size_t>(w) * h, 4096))
        for (uint32 y = 0; y < h; ++y) processRow(y);
    else
        parallelForRows(h, [&](size_t y) { processRow(static_cast<uint32>(y)); }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: custom_grayscale_demo <input-image> <output-image> [serial]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    Image src = iml_example::loadOrDie(argv[1]);

    iml_example::banner("CUSTOM grayscale (kernel)", src);
    std::printf("  backend: %s\n", iml_example::policyName(serial));

    Image gray(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customToGray", pixels, [&]() {
        customToGray(src.view(), gray.view(), policy);
    });

    iml_example::saveOrDie(gray, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}