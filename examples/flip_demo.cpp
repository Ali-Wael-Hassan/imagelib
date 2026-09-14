// examples/flip_demo.cpp
//
// Example 5: Flip image horizontally and vertically (row-parallel copies that
// each thread over the shared thread pool). Both results are written.
//
// Usage: flip_demo <input-image> <output-hflip> <output-vflip> [serial]

#include "example_util.h"

using namespace iml;

int main(int argc, char** argv) {
    if (argc < 4) {
        std::printf("Usage: flip_demo <input-image> <output-hflip> <output-vflip> [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outH = argv[2], outV = argv[3];
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Flip image", src);
    std::printf("  backend: %s\n", iml_example::policyName(serial));
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();

    Image h(src.format(), src.width(), src.height(), src.colorSpace());
    Image v(src.format(), src.width(), src.height(), src.colorSpace());

    iml_example::timeAndReport("flipHorizontal", pixels, [&]() {
        proc::transform::flipHorizontal(src.view(), h.view(), policy);
    });
    iml_example::timeAndReport("flipVertical", pixels, [&]() {
        proc::transform::flipVertical(src.view(), v.view(), policy);
    });

    iml_example::saveOrDie(h, outH);
    iml_example::saveOrDie(v, outV);
    std::printf("  wrote %s, %s\n", outH.c_str(), outV.c_str());
    return 0;
}