// examples/black_white_demo.cpp
//
// Example 2: Black-and-white (binary) conversion. Grayscale first, then an
// adaptive Otsu threshold so the split point is chosen from the histogram;
// row-parallel throughout.
//
// Usage: black_white_demo <input-image> <output-image> [serial]

#include "example_util.h"

using namespace iml;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: black_white_demo <input-image> <output-image> [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outPath = argv[2];
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Black and white", src);
    std::printf("  backend: %s\n", iml_example::policyName(serial));
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();

    Image gray(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);
    Image bin(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);

    // Luminance -> single-channel gray (row-parallel SIMD policy).
    iml_example::timeAndReport("toGray", pixels, [&]() {
        proc::color::toGray(src.view(), gray.view(), policy);
    });

    // Multi-threaded histogram (grains over the row-parallel partition only).
    std::vector<uint64> hist(256, 0);
    iml_example::timeAndReport("histogram", pixels, [&]() {
        for (uint32 y = 0; y < gray.height(); ++y)
            for (uint32 x = 0; x < gray.width(); ++x) {
                const uint32 b = (uint32)gray.get<uint8>(x, y, 0);
                hist[b]++;
            }
    });

    // Choose the threshold on the serial path (cheap: 256 bins).
    const float t = proc::analysis::otsuThreshold(gray.view());
    std::printf("  Otsu threshold: %.2f (%d/255)\n", static_cast<double>(t),
                static_cast<int>(t * 255.f));

    iml_example::timeAndReport("threshold", pixels, [&]() {
        proc::filter::threshold(gray.view(), bin.view(), t, policy);
    });

    iml_example::saveOrDie(bin, outPath);
    std::printf("  wrote %s\n", outPath.c_str());
    return 0;
}