// examples/custom/custom_resize_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #11: Resize)
// ======================================================================
//
// What you learn here:
//   * Resize is a geometric remap just like rotate: each output pixel samples
//     the source at a fractional position. The scale factors map every output
//     (x, y) into source space via `fx = (x + 0.5) * sw/dw - 0.5`. The +0.5/-0.5
//     keeps the image centered instead of snapping to the top-left pixel.
//   * We reuse bilinear interpolation (the same helper as custom_rotate) —
//     code reuse across filters is exactly what shared DAG-style kernels enable.
//     dst channels can differ from src (e.g. gray output), so we respect the
//     destination's channel count.
//
// How it is OPTIMIZED:
//   * The scale ratio is computed ONCE (outside the pixel loop) as a float.
//     Integer division inside the loop would be ~20x slower per pixel.
//   * Row-parallel. Per row, `fy` is constant, so we precompute it and the row
//     weights (y0, y1, ty) once — the compiler hoists that out of the x loop.
//
// Usage: custom_resize_demo <input> <output> [serial] [width] [height]

#include "../example_util.h"

using namespace iml;

namespace {

/// Bilinear sample (reused from custom_rotate_demo).
inline float bilinearNorm(const ConstImageView& src, float fx, float fy, uint32 c) {
    const int32 w = static_cast<int32>(src.width());
    const int32 h = static_cast<int32>(src.height());
    const int32 x0 = static_cast<int32>(std::floor(fx));
    const int32 y0 = static_cast<int32>(std::floor(fy));
    const int32 x1 = x0 + 1, y1 = y0 + 1;
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);
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

inline void customResize(const ConstImageView& src, ImageView dst,
                         const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customResize: invalid view");

    const uint32 dw = dst.width(), dh = dst.height(), ch = dst.channels();
    // Centered, non-flipped source mapping. Denominators are >= 1 (nonzero size).
    const float sx = static_cast<float>(src.width())  / static_cast<float>(dw);
    const float sy = static_cast<float>(src.height()) / static_cast<float>(dh);
    const float offx = 0.5f * (sx - 1.f);
    const float offy = 0.5f * (sy - 1.f);

    auto processRow = [&](uint32 y) {
        const float fy = static_cast<float>(y) * sy + offy;
        for (uint32 x = 0; x < dw; ++x) {
            const float fx = static_cast<float>(x) * sx + offx;
            for (uint32 c = 0; c < ch; ++c)
                proc::detail::writeNorm(dst, x, y, c, bilinearNorm(src, fx, fy, c));
        }
    };

    if (!iml::detail::wantsParallel(policy, static_cast<size_t>(dw) * dh, 4096))
        for (uint32 y = 0; y < dh; ++y) processRow(y);
    else
        parallelForRows(dh, [&](size_t y) { processRow(static_cast<uint32>(y)); }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: custom_resize_demo <input-image> <output-image> [serial] [width] [height]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    Image src = iml_example::loadOrDie(argv[1]);

    const uint32 outW = argc >= 5 ? static_cast<uint32>(std::atoi(argv[4]))
                                  : static_cast<uint32>(src.width() * 1.5f);
    const uint32 outH = argc >= 6 ? static_cast<uint32>(std::atoi(argv[5]))
                                  : static_cast<uint32>(src.height() * 1.5f);

    iml_example::banner("CUSTOM resize (kernel)", src);
    std::printf("  backend: %s | %ux%u -> %ux%u\n",
                iml_example::policyName(serial), src.width(), src.height(), outW, outH);

    Image out(src.format(), outW, outH, src.colorSpace());
    const uint64 pixels = static_cast<uint64>(outW) * outH;
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customResize", pixels, [&]() {
        customResize(src.view(), out.view(), policy);
    });

    iml_example::saveOrDie(out, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}