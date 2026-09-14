// examples/resize_demo.cpp
//
// Example 11: Resize (upscale and downscale) with bilinear interpolation on the
// row-parallel resize kernel.
//
// Usage: resize_demo <input-image> <output-image> <scale> [serial]
//   scale: multiplier, e.g. 2.0 for 2x (or 0.75 for 75%)

#include "example_util.h"

using namespace iml;

int main(int argc, char** argv) {
    if (argc < 4) {
        std::printf("Usage: resize_demo <input-image> <output-image> <scale> [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outPath = argv[2];
    const float scale = static_cast<float>(std::atof(argv[3]));
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Resize image", src);
    std::printf("  backend: %s, scale: %.2f\n",
                iml_example::policyName(serial), static_cast<double>(scale));

    const uint32 dw = static_cast<uint32>(static_cast<float>(src.width()) * scale);
    const uint32 dh = static_cast<uint32>(static_cast<float>(src.height()) * scale);
    if (dw == 0 || dh == 0) {
        std::printf("error: scale too small\n");
        return 1;
    }
    Image dst(fmt::rgb8, dw, dh, ColorSpace::SRGB);
    Image rg(fmt::rgb8, src.width(), src.height(), ColorSpace::SRGB);
    if (src.channels() != 3)
        proc::color::toGray(src.view(), rg.view(), policy); // converts to luma rgb
    const ConstImageView& rsrc = (src.channels() == 3) ? src.view() : rg.view();

    const uint64 pixels = static_cast<uint64>(dw) * dh;
    iml_example::timeAndReport("resize (bilinear)", pixels, [&]() {
        proc::resize::resize(rsrc, dst.view(), proc::resize::Filter::Bilinear,
                             proc::conv::BorderMode::Clamp, policy);
    });

    iml_example::saveOrDie(dst, outPath);
    std::printf("  %ux%u -> %ux%u, wrote %s\n", src.width(), src.height(), dw, dh,
                outPath.c_str());
    return 0;
}