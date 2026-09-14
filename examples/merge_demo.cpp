// examples/merge_demo.cpp
//
// Example 4: Merge two images. Three modes (select via optional argv[3]):
//   avg     - 50/50 alpha blend of two equal-sized images
//   overlay - 'top' blended at 35% over 'bottom'
//   mosaic  - horizontal side-by-side composite (new canvas)
// The pixel mixers run over the row-parallel partition with the SIMD policy.
//
// Usage: merge_demo <image-a> <image-b> <output-image> [avg|overlay|mosaic] [serial]

#include "example_util.h"

using namespace iml;

namespace {

inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

// out = a*(1-t) + b*t, per channel, across all pixels.
void blendImages(const ConstImageView& a, const ConstImageView& b,
                 ImageView out, float t, const ExecutionPolicy& policy) {
    const uint32 w = a.width(), h = a.height();
    const uint32 ch = std::min<uint32>(a.channels(), b.channels());
    const uint32 width = w;
    parallelForRows(h, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x)
            for (uint32 c = 0; c < ch; ++c) {
                const float va = proc::detail::readNorm(a, x, y, c);
                const float vb = proc::detail::readNorm(b, x, y, c);
                proc::detail::writeNorm(out, x, y, c, lerpf(va, vb, t));
            }
    }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::printf("Usage: merge_demo <image-a> <image-b> <output-image> "
                    "[avg|overlay|mosaic] [serial]\n");
        return 1;
    }
    const std::string inA = argv[1], inB = argv[2], outPath = argv[3];
    std::string mode = argc >= 5 ? argv[4] : "avg";
    if (mode != "avg" && mode != "overlay" && mode != "mosaic") {
        std::printf("unknown mode '%s' (use avg|overlay|mosaic)\n", mode.c_str());
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image a = iml_example::loadOrDie(inA);
    Image b = iml_example::loadOrDie(inB);
    iml_example::banner("Merge images", a);
    std::printf("  backend: %s, mode: %s\n", iml_example::policyName(serial), mode.c_str());

    Image out;
    if (mode == "mosaic") {
        out = Image(a.format(), a.width() + b.width(),
                    std::max(a.height(), b.height()), ColorSpace::SRGB);
        const uint64 pixels = static_cast<uint64>(out.width()) * out.height();
        iml_example::timeAndReport("mosaic compose", pixels, [&]() {
            parallelForRows(out.height(), [&](size_t row) {
                const uint32 y = static_cast<uint32>(row);
                for (uint32 x = 0; x < a.width(); ++x)
                    for (uint32 c = 0; c < a.channels(); ++c)
                        proc::detail::writeNorm(out.view(), x, y, c,
                                               proc::detail::readNorm(a.view(), x, y, c));
                for (uint32 x = 0; x < b.width(); ++x)
                    for (uint32 c = 0; c < b.channels(); ++c)
                        proc::detail::writeNorm(out.view(), a.width() + x, y, c,
                                               proc::detail::readNorm(b.view(), x, y, c));
            }, policy);
        });
    } else {
        if (a.width() != b.width() || a.height() != b.height()) {
            std::printf("error: avg/overlay modes need equal-size images\n");
            return 1;
        }
        out = Image(a.format(), a.width(), a.height(), a.colorSpace());
        const float t = (mode == "overlay") ? 0.35f : 0.5f;
        const uint64 pixels = static_cast<uint64>(a.width()) * a.height();
        iml_example::timeAndReport(t == 0.5f ? "50/50 blend" : "35% overlay",
                                   pixels, [&]() {
            blendImages(a.view(), b.view(), out.view(), t, policy);
        });
    }

    iml_example::saveOrDie(out, outPath);
    std::printf("  wrote %s\n", outPath.c_str());
    return 0;
}