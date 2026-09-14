// examples/grayscale_demo.cpp
//
// Example 1: Grayscale conversion (rec.709 luma) using the row-parallel
// image core. Threading + SIMD policy is supplied by the caller.
//
// Usage: grayscale_demo <input-image> <output-image> [serial]

#include "example_util.h"

using namespace iml;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: grayscale_demo <input-image> <output-image> [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outPath = argv[2];
    const bool serial = iml_example::wantsSerial(argc, argv);
    const iml::ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml::Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Grayscale conversion", src);
    std::printf("  backend: %s\n", iml_example::policyName(serial));

    iml::Image gray(iml::fmt::gray8, src.width(), src.height(), iml::ColorSpace::Gray);
    iml::Image rgb(iml::fmt::rgb8, src.width(), src.height(), iml::ColorSpace::SRGB);
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();

    iml_example::timeAndReport("toGray (color -> rgb8)", pixels, [&]() {
        iml::proc::color::toGray(src.view(), rgb.view(), policy);
    });

    // Route the luminance into a gray8 image as well (canonical spelling).
    iml_example::timeAndReport("toGray (color -> gray8)", pixels, [&]() {
        iml::proc::color::toGray(src.view(), gray.view(), policy);
    });

    iml_example::saveOrDie(gray, outPath);
    std::printf("  wrote %s\n", outPath.c_str());
    return 0;
}