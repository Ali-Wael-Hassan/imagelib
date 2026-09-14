// examples/custom/custom_crop_demo.cpp
//
// HOW TO WRITE A FILTER KERNEL YOURSELF (Assignment Example #8: Crop)
// ==================================================================
//
// What you learn here:
//   * Crop is the simplest possible remap: an axis-aligned rectangular copy.
//     dst[x][y] = src[x0 + x][y0 + y]. No interpolation, no neighbors.
//   * The destination is SMALLER than the source, which is the key difference
//     from the other kernels so far: the sizes are parameters, not derived
//     from src. Always validate that the requested box fits inside src.
//   * OOP: the rectangle (x0, y0, w, h) is a single value object. AutoCAD-style
//     "pass one rect instead of four ints" prevents ordering bugs.
//
// How it is OPTIMIZED:
//   * Row-parallel copies: each row is independent, and mem::copy moves whole
//     rows as wide copies (much faster than per-pixel loops).
//   * This is as fast as cropping can physically be — pure memory bandwidth.
//
// Usage: custom_crop_demo <input> <output> [serial] [x y width height]

#include "../example_util.h"

using namespace iml;

namespace {

inline void customCrop(const ConstImageView& src, ImageView dst,
                       Rect region, const ExecutionPolicy& policy) {
    if (!src.valid() || !dst.valid())
        throw InvalidParameterError("customCrop: invalid view");
    if (region.x < 0 || region.y < 0 ||
        region.x + static_cast<int32>(region.width) > static_cast<int32>(src.width()) ||
        region.y + static_cast<int32>(region.height) > static_cast<int32>(src.height()))
        throw InvalidParameterError("customCrop: region exceeds source bounds");
    if (region.width != dst.width() || region.height != dst.height())
        throw InvalidParameterError("customCrop: region/size mismatch");

    const uint32 w = region.width;
    const uint32 colBytes = static_cast<uint32>(src.pixelSizeBytes());

    auto processRow = [&](uint32 y) {
        const byte* const sp = src.row(region.y + static_cast<int32>(y))
                            + static_cast<uint64>(region.x) * colBytes;
        mem::copy(dst.row(static_cast<int32>(y)), sp, static_cast<size_t>(w) * colBytes);
    };

    if (!iml::detail::wantsParallel(policy, static_cast<size_t>(w) * region.height, 4096))
        for (uint32 y = 0; y < region.height; ++y) processRow(y);
    else
        parallelForRows(static_cast<size_t>(region.height),
                        [&](size_t y) { processRow(static_cast<uint32>(y)); }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: custom_crop_demo <input-image> <output-image> [serial] [x y w h]\n");
        return 1;
    }
    const bool serial = iml_example::wantsSerial(argc, argv);
    Image src = iml_example::loadOrDie(argv[1]);

    Rect region;
    if (argc >= 7) {
        region = Rect(std::atoi(argv[3]), std::atoi(argv[4]),
                      static_cast<uint32>(std::atoi(argv[5])),
                      static_cast<uint32>(std::atoi(argv[6])));
    } else { // default: central 60%
        const uint32 w = src.width(), h = src.height();
        region = Rect(static_cast<int32>(w / 5), static_cast<int32>(h / 5),
                      w * 3 / 5, h * 3 / 5);
    }

    iml_example::banner("CUSTOM crop (kernel)", src);
    std::printf("  backend: %s | region: (%d,%d) %ux%u\n",
                iml_example::policyName(serial),
                region.x, region.y, region.width, region.height);

    Image out(src.format(), region.width, region.height, src.colorSpace());
    const uint64 pixels = static_cast<uint64>(region.width) * region.height;
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    iml_example::timeAndReport("customCrop", pixels, [&]() {
        customCrop(src.view(), out.view(), region, policy);
    });

    iml_example::saveOrDie(out, argv[2]);
    std::printf("  wrote %s\n", argv[2]);
    return 0;
}