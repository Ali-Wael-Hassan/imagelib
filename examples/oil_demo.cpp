// examples/oil_demo.cpp
//
// Example 13: Oil-painting effect. For each pixel, quantize the luminance of
// its neighbourhood into `levels` bins, find the most populated bin, and emit
// the average colour of the pixels in that bin. Fully row-parallel; the
// per-pixel histograms live on the stack of each worker.
//
// Usage: oil_demo <input-image> <output-image> [radius] [levels] [serial]

#include "example_util.h"

using namespace iml;

namespace {

void oilPaint(const ConstImageView& src, ImageView dst, int radius, int levels,
              const ExecutionPolicy& policy) {
    const uint32 r = static_cast<uint32>(radius);
    const uint32 w = src.width(), h = src.height();
    const uint32 ch = std::min<uint32>(src.channels(), dst.channels());
    const uint32 width = w, height = h, rr = r, lv = static_cast<uint32>(levels);
    parallelForRows(height, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x) {
            // Bins accumulate (count, sumR, sumG, sumB).
            struct Bin { uint32 count = 0; float sr = 0, sg = 0, sb = 0; };
            std::vector<Bin> bins(lv);
            const int32 y0 = static_cast<int32>(y) - static_cast<int32>(rr);
            const int32 y1 = static_cast<int32>(y) + static_cast<int32>(rr) + 1;
            const int32 x0 = static_cast<int32>(x) - static_cast<int32>(rr);
            const int32 x1 = static_cast<int32>(x) + static_cast<int32>(rr) + 1;
            for (int32 yy = y0; yy < y1; ++yy) {
                for (int32 xx = x0; xx < x1; ++xx) {
                    const float r0 = proc::conv::sampleNorm(src, xx, yy, 0, proc::conv::BorderMode::Clamp);
                    const float l = r0;
                    uint32 b = static_cast<uint32>(l * static_cast<float>(lv));
                    if (b >= lv) b = lv - 1;
                    Bin& bin = bins[b];
                    ++bin.count;
                    bin.sr += r0;
                    if (ch > 1) bin.sg += proc::conv::sampleNorm(src, xx, yy, 1, proc::conv::BorderMode::Clamp);
                    if (ch > 2) bin.sb += proc::conv::sampleNorm(src, xx, yy, 2, proc::conv::BorderMode::Clamp);
                }
            }
            // Most populous bin.
            uint32 best = 0;
            for (uint32 b = 1; b < lv; ++b)
                if (bins[b].count > bins[best].count) best = b;
            const Bin& out = bins[best];
            const float inv = out.count > 0 ? 1.f / static_cast<float>(out.count) : 0.f;
            proc::detail::writeNorm(dst, x, y, 0, out.sr * inv);
            if (ch > 1) proc::detail::writeNorm(dst, x, y, 1, out.sg * inv);
            if (ch > 2) proc::detail::writeNorm(dst, x, y, 2, out.sb * inv);
        }
    }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: oil_demo <input-image> <output-image> [radius] [levels] [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outPath = argv[2];
    const int radius = argc >= 4 ? std::max(1, std::atoi(argv[3])) : 4;
    const int levels = argc >= 5 ? std::max(2, std::atoi(argv[4])) : 16;
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Oil painting", src);
    std::printf("  backend: %s, radius: %d, levels: %d\n",
                iml_example::policyName(serial), radius, levels);

    Image dst(fmt::rgb8, src.width(), src.height(), ColorSpace::SRGB);
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();
    iml_example::timeAndReport("oil paint", pixels, [&]() {
        oilPaint(src.view(), dst.view(), radius, levels, policy);
    });

    iml_example::saveOrDie(dst, outPath);
    std::printf("  wrote %s\n", outPath.c_str());
    return 0;
}