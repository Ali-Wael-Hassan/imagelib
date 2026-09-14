// examples/invert_demo.cpp
//
// Example 3: Per-pixel colour inversion. One SIMD-parallel pass over every
// pixel using the filter kernel: out = 1 - in.
//
// Usage: invert_demo <input-image> <output-image> [serial]

#include "example_util.h"

using namespace iml;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: invert_demo <input-image> <output-image> [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outPath = argv[2];
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Image inversion", src);
    std::printf("  backend: %s\n", iml_example::policyName(serial));
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();

    Image dst(src.format(), src.width(), src.height(), src.colorSpace());

    iml_example::timeAndReport("filter::invert", pixels, [&]() {
        proc::filter::invert(src.view(), dst.view(), policy);
    });

    iml_example::saveOrDie(dst, outPath);
    std::printf("  wrote %s\n", outPath.c_str());
    return 0;
}