// examples/darken_lighten_demo.cpp
//
// Example 7: Darken and lighten the image. Uses the brightness kernel with a
// negative (darken) and positive (lighten) delta; both passes are row-parallel.
//
// Usage: darken_lighten_demo <input-image> <output-dark> <output-light> [serial]

#include "example_util.h"

using namespace iml;

int main(int argc, char** argv) {
    if (argc < 4) {
        std::printf("Usage: darken_lighten_demo <input-image> <output-dark> <output-light> [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outDark = argv[2], outLight = argv[3];
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Darken / lighten", src);
    std::printf("  backend: %s\n", iml_example::policyName(serial));
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();

    Image dark(src.format(), src.width(), src.height(), src.colorSpace());
    Image light(src.format(), src.width(), src.height(), src.colorSpace());

    iml_example::timeAndReport("darken (delta -0.30)", pixels, [&]() {
        proc::filter::brightness(src.view(), dark.view(), -0.30f, policy);
    });
    iml_example::timeAndReport("lighten (delta +0.20)", pixels, [&]() {
        proc::filter::brightness(src.view(), light.view(), +0.20f, policy);
    });

    iml_example::saveOrDie(dark, outDark);
    iml_example::saveOrDie(light, outLight);
    std::printf("  wrote %s, %s\n", outDark.c_str(), outLight.c_str());
    return 0;
}