// examples/custom/custom_edge_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #10: Edge detect)
// ===========================================================================
//
// What you learn here:
//   * Edges are places where the image changes quickly. We measure the change
//     with two 3x3 first-derivative kernels (Sobel X and Sobel Y) and output
//     the gradient MAGNITUDE: sqrt(gx^2 + gy^2). Strong edges -> white.
//   * This is a NEIGHBORHOOD operator: the output pixel depends on the 3x3 box
//     around it. The box is why we need border handling (the edges of the
//     image), solved here with sampleNorm + BorderMode::Clamp.
//   * Key OOP/design insight: the filter is parameterized by its kernels. You
//     could pass any 3x3 weights (sharpen, emboss...) through the same kernel.
//
// How it is OPTIMIZED:
//   * Row-parallel, and all border checks are pushed into sampleNorm so the
//     inner loop is straight nine-tap math the compiler can vectorize.
//   * The gx/gy weighths are constants in registers — no table lookups.
//
// Usage: custom_edge_demo <input> <output> [serial]

#include "../example_util.h"
#include "imagelib/processing/Convolution.h"

using namespace iml;

namespace {

inline float grayscaleN(const ConstImageView& src, int32 sx, int32 sy) {
    if (src.channels() >= 3) {
        const float r = proc::conv::sampleNorm(src, sx, sy, 0, proc::conv::BorderMode::Clamp);
        const float g = proc::conv::sampleNorm(src, sx, sy, 1, proc::conv::BorderMode::Clamp);
        const float b = proc::conv::sampleNorm(src, sx, sy, 2, proc::conv::BorderMode::Clamp);
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }
    return proc::conv::sampleNorm(src, sx, sy, 0, proc::conv::BorderMode::Clamp);
}

inline void customSobel(const ConstImageView& src, ImageView dst,
                        const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customSobel: invalid view");

    const int32 w = static_cast<int32>(src.width());
    const int32 h = static_cast<int32>(src.height());

    auto processRow = [&](int32 y) {
        for (int32 x = 0; x < w; ++x) {
            // 3x3 neighborhood (normalized), clamped at the borders.
            const float p[3][3] = {
                { grayscaleN(src, x - 1, y - 1), grayscaleN(src, x, y - 1), grayscaleN(src, x + 1, y - 1) },
                { grayscaleN(src, x - 1, y),     grayscaleN(src, x, y),     grayscaleN(src, x + 1, y)     },
                { grayscaleN(src, x - 1, y + 1), grayscaleN(src, x, y + 1), grayscaleN(src, x + 1, y + 1) },
            };
            const float gx = (p[0][2] + 2.f * p[1][2] + p[2][2])
                           - (p[0][0] + 2.f * p[1][0] + p[2][0]);
            const float gy = (p[2][0] + 2.f * p[2][1] + p[2][2])
                           - (p[0][0] + 2.f * p[0][1] + p[0][2]);
            const float mag = std::sqrt(gx * gx + gy * gy);
            proc::detail::writeNorm(dst, x, y, 0, mag < 1.f ? mag : 1.f);
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
        std::printf("Usage: custom_edge_demo <input-image> <output-image> [serial]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    Image src = iml_example::loadOrDie(argv[1]);

    iml_example::banner("CUSTOM Sobel edge (kernel)", src);
    std::printf("  backend: %s\n", iml_example::policyName(serial));

    Image out(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customSobel", pixels, [&]() {
        customSobel(src.view(), out.view(), policy);
    });

    iml_example::saveOrDie(out, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}