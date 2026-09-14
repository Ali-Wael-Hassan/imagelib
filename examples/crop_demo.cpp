// examples/crop_demo.cpp
//
// Example 8: Crop a rectangular region. ROI copy is row-parallel. Region is
// taken from argv[4..7] as x y width height (0-based), or defaults to the
// center 50% x 50%.
//
// Usage: crop_demo <input-image> <output-image> [x y width height] [serial]

#include "example_util.h"

using namespace iml;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: crop_demo <input-image> <output-image> [x y width height] [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outPath = argv[2];
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Crop image", src);
    std::printf("  backend: %s\n", iml_example::policyName(serial));

    uint32 rx, ry, rw, rh;
    if (argc >= 7) {
        rx = static_cast<uint32>(std::atoi(argv[3]));
        ry = static_cast<uint32>(std::atoi(argv[4]));
        rw = static_cast<uint32>(std::atoi(argv[5]));
        rh = static_cast<uint32>(std::atoi(argv[6]));
    } else {
        rw = src.width() / 2;
        rh = src.height() / 2;
        rx = (src.width() - rw) / 2;
        ry = (src.height() - rh) / 2;
    }
    std::printf("  region x=%u y=%u w=%u h=%u\n", rx, ry, rw, rh);
    if (rx + rw > src.width() || ry + rh > src.height() || rw == 0 || rh == 0) {
        std::printf("error: invalid crop region\n");
        return 1;
    }
    const uint64 pixels = static_cast<uint64>(rw) * rh;

    Image dst(src.format(), rw, rh, src.colorSpace());
    iml_example::timeAndReport("copyRoi (crop)", pixels, [&]() {
        proc::transform::copyRoi(src.view(), dst.view(), rx, ry, rw, rh, policy);
    });

    iml_example::saveOrDie(dst, outPath);
    std::printf("  wrote %s\n", outPath.c_str());
    return 0;
}