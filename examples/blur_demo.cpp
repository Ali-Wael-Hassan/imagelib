// examples/blur_demo.cpp
//
// Example 12: Gaussian blur. The separable filter runs two row-parallel 1D
// passes (horizontal then vertical) with a float accumulation image in
// between. Sigma defaults to 3.0.
//
// Usage: blur_demo <input-image> <output-image> [sigma] [serial]

#include "example_util.h"

using namespace iml;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: blur_demo <input-image> <output-image> [sigma] [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outPath = argv[2];
    const float sigma = argc >= 4 ? static_cast<float>(std::atof(argv[3])) : 3.0f;
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Gaussian blur", src);
    std::printf("  backend: %s, sigma: %.1f\n",
                iml_example::policyName(serial), static_cast<double>(sigma));
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();

    Image dst(fmt::rgb8, src.width(), src.height(), ColorSpace::SRGB);
    // Separable blur: two row-parallel 1D passes (horizontal, then vertical).
    iml_example::timeAndReport("gaussianBlur (2 sep passes)", pixels, [&]() {
        proc::conv::gaussianBlur(src.view(), dst.view(), sigma,
                                 proc::conv::BorderMode::Clamp, policy);
    });

    iml_example::saveOrDie(dst, outPath);
    std::printf("  wrote %s\n", outPath.c_str());
    return 0;
}