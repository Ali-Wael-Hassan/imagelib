// examples/custom/custom_frame_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #9: Frame)
// ====================================================================
//
// What you learn here:
//   * A frame paints a solid border of `thickness` pixels around a copy of the
//     source. The middle of the image is untouched: only the edge bands change.
//   * Row-wise this splits into THREE zones: top band (fill whole row),
//     middle band (fill only the first/last columns), bottom band (fill whole
//     row). Thinking in row zones is exactly how you would parallelize any
//     border-eroding/dilating effect.
//   * The frame color is a parameter (r,g,b as a small value object), and the
//     fill color is passed pointwise — no global paint state. Reusable.
//
// How it is OPTIMIZED:
//   * A single parallelForRows drives all three zones, so the thread pool is
//     shared instead of spawning three nested loops.
//   * The border test is hoisted per-row (y<thickness etc.), not recomputed
//     inside the pixel loop.
//
// Usage: custom_frame_demo <input> <output> [serial] [thickness] [r] [g] [b]

#include "../example_util.h"

using namespace iml;

namespace {

inline void customFrame(const ConstImageView& src, ImageView dst, uint32 thickness,
                        float fr, float fg, float fb, const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customFrame: invalid view");
    if (thickness == 0 || thickness * 2 > dst.width() || thickness * 2 > dst.height())
        throw InvalidParameterError("customFrame: invalid thickness");

    const uint32 w = dst.width(), h = dst.height(), ch = dst.channels();
    const uint32 colBytes = src.pixelSizeBytes();

    auto fillPixel = [&](uint32 x, uint32 y) {
        for (uint32 c = 0; c < ch; ++c) {
            const float col = (c == 0) ? fr : (c == 1) ? fg : (c == 2) ? fb : 1.f;
            proc::detail::writeNorm(dst, x, y, c, col);
        }
    };

    auto processRow = [&](uint32 y) {
        // Start from the source row ...
        mem::copy(dst.row(static_cast<int32>(y)), src.row(static_cast<int32>(y)),
                  static_cast<size_t>(w) * colBytes);
        // ... then paint only the border pixels.
        if (y < thickness || y >= h - thickness) {
            for (uint32 x = 0; x < w; ++x) fillPixel(x, y);   // top / bottom band
        } else {
            for (uint32 x = 0; x < thickness; ++x) fillPixel(x, y);             // left
            for (uint32 x = w - thickness; x < w; ++x) fillPixel(x, y);         // right
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
        std::printf("Usage: custom_frame_demo <input-image> <output-image> [serial] [thickness] [r] [g] [b]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    const uint32 thickness = argc >= 5 ? static_cast<uint32>(std::atoi(argv[4])) : 8u;
    const float fr = argc >= 6 ? static_cast<float>(std::atof(argv[5])) : 1.f;
    const float fg = argc >= 7 ? static_cast<float>(std::atof(argv[6])) : 0.f;
    const float fb = argc >= 8 ? static_cast<float>(std::atof(argv[7])) : 0.f;
    Image src = iml_example::loadOrDie(argv[1]);

    iml_example::banner("CUSTOM frame (kernel)", src);
    std::printf("  backend: %s | frame: %upx, color (%.1f, %.1f, %.1f)\n",
                iml_example::policyName(serial), thickness, fr, fg, fb);

    Image out(src.format(), src.width(), src.height(), src.colorSpace());
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customFrame", pixels, [&]() {
        customFrame(src.view(), out.view(), thickness, fr, fg, fb, policy);
    });

    iml_example::saveOrDie(out, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}