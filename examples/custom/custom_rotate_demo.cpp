// examples/custom/custom_rotate_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #6: Rotate)
// =====================================================================
//
// What you learn here:
//   * Rotating by an arbitrary angle is a GEOMERMAPPING: for each destination
//     pixel we ask "which source pixel does this come from?" (inverse mapping).
//     Because 1 source pixel no longer maps exactly onto 1 destination pixel,
//     we sample with bilinear interpolation: average the 4 neighbors weighted
//     by the fractional offsets.
//   * This is the difference between:
//       - point operators (grayscale/invert): same (x,y) everywhere;
//       - neighborhood operators (this one, blur, edge): sample around (x,y);
//       - geometric remapping (this one, resize): arbitrary (x', y').
//   * Border handling: coordinates that fall outside the source are clamped
//     (BorderMode::Clamp). Try passing Wrap/Mirror in sampleNorm() yourself.
//
// How it is OPTIMIZED:
//   * parallelForRows: every output row is independent (we never read dst).
//   * All math is float; the samples are computed with constants hoisted out
//     of the inner loop, so each output pixel costs 4 reads + few FMAs.
//
// Usage: custom_rotate_demo <input> <output> [serial] [degrees]

#include "../example_util.h"

using namespace iml;

namespace {

inline constexpr float kDegToRad = 0.017453292519943295f;

/// Bilinear sample of a normalized channel at fractional coordinates.
inline float bilinearNorm(const ConstImageView& src, float fx, float fy, uint32 c) {
    const int32 w = static_cast<int32>(src.width());
    const int32 h = static_cast<int32>(src.height());
    const int32 x0 = static_cast<int32>(std::floor(fx));
    const int32 y0 = static_cast<int32>(std::floor(fy));
    const int32 x1 = x0 + 1, y1 = y0 + 1;
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);

    // Border clamp is applied per sample, so edges never read out of bounds.
    const auto S = [&](int32 x, int32 y) {
        x = std::min(std::max(x, 0), w - 1);
        y = std::min(std::max(y, 0), h - 1);
        return proc::detail::readNorm(src, static_cast<uint32>(x), static_cast<uint32>(y), c);
    };

    const float v00 = S(x0, y0), v10 = S(x1, y0);
    const float v01 = S(x0, y1), v11 = S(x1, y1);
    const float top = v00 * (1.f - tx) + v10 * tx;
    const float bot = v01 * (1.f - tx) + v11 * tx;
    return top * (1.f - ty) + bot * ty;
}

inline void customRotate(const ConstImageView& src, ImageView dst, float degrees,
                         const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customRotate: invalid view");

    const float rad = degrees * kDegToRad;
    const float c = std::cos(rad), s = std::sin(rad);
    const uint32 w = dst.width(), h = dst.height(), ch = dst.channels();
    // Map destination pixel (x,y), centered, back into source space.
    const float cx = 0.5f * static_cast<float>(w);
    const float cy = 0.5f * static_cast<float>(h);
    const float scx = 0.5f * static_cast<float>(src.width());
    const float scy = 0.5f * static_cast<float>(src.height());

    auto processRow = [&](uint32 y) {
        const float v = static_cast<float>(y) - cy;
        for (uint32 x = 0; x < w; ++x) {
            const float u = static_cast<float>(x) - cx;
            const float fx = c * u + s * v + scx; // inverse rotation
            const float fy = -s * u + c * v + scy;
            for (uint32 cc = 0; cc < ch; ++cc)
                proc::detail::writeNorm(dst, x, y, cc, bilinearNorm(src, fx, fy, cc));
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
        std::printf("Usage: custom_rotate_demo <input-image> <output-image> [serial] [degrees]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    const float degrees = argc >= 5 ? static_cast<float>(std::atof(argv[4])) : 30.f;
    Image src = iml_example::loadOrDie(argv[1]);

    iml_example::banner("CUSTOM rotate (kernel)", src);
    std::printf("  backend: %s | angle: %.0f deg\n",
                iml_example::policyName(serial), degrees);

    Image out(src.format(), src.width(), src.height(), src.colorSpace());
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customRotate", pixels, [&]() {
        customRotate(src.view(), out.view(), degrees, policy);
    });

    iml_example::saveOrDie(out, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}