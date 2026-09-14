// examples/frame_demo.cpp
//
// Example 9: Add a solid color frame (border) around the picture. The border
// fill runs over every row in parallel, then the original image is copied into
// the centre with a row-parallel ROI copy.
//
// Usage: frame_demo <input-image> <output-image> [border-pixels] [r g b] [serial]

#include "example_util.h"

using namespace iml;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: frame_demo <input-image> <output-image> "
                    "[border-pixels] [r g b] [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outPath = argv[2];
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Frame picture", src);
    std::printf("  backend: %s\n", iml_example::policyName(serial));

    const uint32 border = argc >= 4 ? static_cast<uint32>(std::max(0, std::atoi(argv[3]))) : 24u;
    float fr = 0.02f, fg = 0.02f, fb = 0.25f; // dark blue by default
    if (argc >= 7) {
        fr = static_cast<float>(std::atoi(argv[4])) / 255.f;
        fg = static_cast<float>(std::atoi(argv[5])) / 255.f;
        fb = static_cast<float>(std::atoi(argv[6])) / 255.f;
    }
    const uint32 w = src.width(), h = src.height();
    const uint32 fw = w + 2 * border, fh = h + 2 * border;
    std::printf("  frame: %u px, colour (%d, %d, %d), canvas %ux%u\n",
                border, static_cast<int>(fr * 255.f),
                static_cast<int>(fg * 255.f), static_cast<int>(fb * 255.f),
                fw, fh);
    const uint64 pixels = static_cast<uint64>(fw) * fh;

    Image framed(fmt::rgb8, fw, fh, ColorSpace::SRGB);

    // 1) Fill every pixel with the frame colour (row-parallel).
    iml_example::timeAndReport("fill frame colour", pixels, [&]() {
        const uint32 W = fw, H = fh;
        parallelForRows(H, [&](size_t row) {
            for (uint32 x = 0; x < W; ++x) {
                proc::detail::writeNorm(framed.view(), x, (uint32)row, 0, fr);
                proc::detail::writeNorm(framed.view(), x, (uint32)row, 1, fg);
                proc::detail::writeNorm(framed.view(), x, (uint32)row, 2, fb);
            }
        }, policy);
    });

    // 2) Blit the original into the centre.
    ImageView inner = framed.view().subView(border, border, w, h);
    iml_example::timeAndReport("copy image to centre", pixels, [&]() {
        proc::transform::copyRoi(src.view(), inner, 0, 0, w, h, policy);
    });

    iml_example::saveOrDie(framed, outPath);
    std::printf("  wrote %s\n", outPath.c_str());
    return 0;
}