// examples/custom/custom_blur_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #12: Gaussian blur)
// ============================================================================
//
// What you learn here:
//   * A Gaussian blur is a SEPARABLE filter: instead of one 2D NxN pass, you
//     do two 1D passes (horizontal then vertical). Each pass reads only one
//     row/column of neighbors, so it is O(N * width) per row instead of O(N^2
//     * width). That is a huge win: a 7x7 2D blur is 49 operations; the 1D
//     separable version is 7+7 = 14.
//   * Gaussian weights are precomputed once from exp(-r^2 / (2 sigma^2)). The
//     precomputation is in main, outside the hot path — OOP separation of
//     setup vs. performance-critical work.
//   * The intermediate buffer holds Float32 values so no precision is lost
//     between the two passes. We read back with readNorm on a Float32 buffer,
//     which returns the raw value (no additional scaling).
//
// How it is OPTIMIZED:
//   * Precomputed weights: each sample multiplies by a register constant.
//   * Border handling is pushed into sampleNorm (integer + Clamp), so the inner
//     loop is pure float FMA the compiler can auto-vectorize.
//   * Both passes run through parallelForRows.
//
// Usage: custom_blur_demo <input> <output> [serial] [radius]

#include "../example_util.h"

using namespace iml;
using namespace iml::proc::conv;

#include <vector>

namespace {

/// 1D horizontal Gaussian pass into a Float32 temp buffer.
inline void blurHorizontal(const ConstImageView& src, ImageView tmp,
                           const std::vector<float>& weights, int radius,
                           const ExecutionPolicy& policy) {
    const int32 w = static_cast<int32>(src.width());
    const int32 h = static_cast<int32>(src.height());
    const uint32 ch = std::min<uint32>(src.channels(), 4u); // cap at 4

    auto processRow = [&](int32 y) {
        for (int32 x = 0; x < w; ++x) {
            for (uint32 c = 0; c < ch; ++c) {
                float acc = 0.f;
                for (int32 k = -radius; k <= radius; ++k)
                    acc += weights[k + radius] *
                           sampleNorm(src, x + k, y, c, BorderMode::Clamp);
                proc::detail::writeNorm(tmp, x, y, c, acc);
            }
        }
    };

    if (!iml::detail::wantsParallel(policy, static_cast<size_t>(w) * h, 4096))
        for (int32 y = 0; y < h; ++y) processRow(y);
    else
        parallelForRows(static_cast<size_t>(h),
                        [&](size_t y) { processRow(static_cast<int32>(y)); }, policy);
}

/// 1D vertical Gaussian pass from Float32 temp into the final output.
inline void blurVertical(const ConstImageView& tmp, ImageView dst,
                         const std::vector<float>& weights, int radius,
                         const ExecutionPolicy& policy) {
    const int32 w = static_cast<int32>(dst.width());
    const int32 h = static_cast<int32>(dst.height());
    const uint32 ch = std::min<uint32>(dst.channels(), 4u);

    auto processRow = [&](int32 y) {
        for (int32 x = 0; x < w; ++x) {
            for (uint32 c = 0; c < ch; ++c) {
                float acc = 0.f;
                for (int32 k = -radius; k <= radius; ++k) {
                    const int32 sy = std::min(std::max(y + k, 0), h - 1);
                    acc += weights[k + radius] *
                           proc::detail::readNorm(tmp, x, sy, c);
                }
                proc::detail::writeNorm(dst, x, y, c, acc);
            }
        }
    };

    if (!iml::detail::wantsParallel(policy, static_cast<size_t>(w) * h, 4096))
        for (int32 y = 0; y < h; ++y) processRow(y);
    else
        parallelForRows(static_cast<size_t>(h),
                        [&](size_t y) { processRow(static_cast<int32>(y)); }, policy);
}

inline void customGaussianBlur(const ConstImageView& src, ImageView dst, int radius,
                               const std::vector<float>& weights,
                               const ExecutionPolicy& policy) {
    // Float32 temp preserves full precision between the two passes and matches
    // the source channel layout.
    Image tmp(ImageFormat(src.pixelFormat(), DataType::Float32),
              src.width(), src.height(), src.colorSpace());

    blurHorizontal(src, tmp.view(), weights, radius, policy);
    blurVertical(tmp.view(), dst, weights, radius, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: custom_blur_demo <input-image> <output-image> [serial] [radius]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    const int radius = argc >= 5 ? std::atoi(argv[4]) : 3;
    Image src = iml_example::loadOrDie(argv[1]);

    // Precompute Gaussian weights once (OOP: setup outside the hot path).
    const float sigma = static_cast<float>(radius) * 0.5f + 0.5f;
    const float twoSigmaSq = 2.f * sigma * sigma;
    std::vector<float> weights(static_cast<size_t>(radius * 2 + 1));
    float wSum = 0.f;
    for (int k = -radius; k <= radius; ++k) {
        const float w = std::exp(static_cast<float>(-(k * k)) / twoSigmaSq);
        weights[k + radius] = w;
        wSum += w;
    }
    for (float& v : weights) v /= wSum; // normalize to sum to 1

    iml_example::banner("CUSTOM Gaussian blur (separable)", src);
    std::printf("  backend: %s | radius: %d, sigma: %.1f\n",
                iml_example::policyName(serial), radius, sigma);

    Image out(src.format(), src.width(), src.height(), src.colorSpace());
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customGaussianBlur", pixels, [&]() {
        customGaussianBlur(src.view(), out.view(), radius, weights, policy);
    });

    iml_example::saveOrDie(out, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}