// examples/edge_demo.cpp
//
// Example 10: Edge detection via the Sobel operator. This implementation runs
// a single row-parallel pass that computes BOTH gradient components for every
// pixel and writes the magnitude (no temporary images), using the Sobel 3x3
// kernels with clamp border handling.
//
// Usage: edge_demo <input-image> <output-image> [angle-threshold-degrees] [serial]

#include "example_util.h"

using namespace iml;

namespace {

// Sobel magnitude for one pixel (single fused pass).
inline float sobelMagnitude(const ConstImageView& src, int32 x, int32 y) {
    // Gx kernel:   [-1 0 +1; -2 0 +2; -1 0 +1]
    // Gy kernel:   [+1 +2 +1;  0 0  0; -1 -2 -1]
    const float tl = proc::conv::sampleNorm(src, x - 1, y - 1, 0, proc::conv::BorderMode::Clamp);
    const float tc = proc::conv::sampleNorm(src, x,     y - 1, 0, proc::conv::BorderMode::Clamp);
    const float tr = proc::conv::sampleNorm(src, x + 1, y - 1, 0, proc::conv::BorderMode::Clamp);
    const float ml = proc::conv::sampleNorm(src, x - 1, y,     0, proc::conv::BorderMode::Clamp);
    const float mr = proc::conv::sampleNorm(src, x + 1, y,     0, proc::conv::BorderMode::Clamp);
    const float bl = proc::conv::sampleNorm(src, x - 1, y + 1, 0, proc::conv::BorderMode::Clamp);
    const float bc = proc::conv::sampleNorm(src, x,     y + 1, 0, proc::conv::BorderMode::Clamp);
    const float br = proc::conv::sampleNorm(src, x + 1, y + 1, 0, proc::conv::BorderMode::Clamp);

    const float gx = -tl + tr - 2.f * ml + 2.f * mr - bl + br;
    const float gy =  tl + 2.f * tc + tr - bl - 2.f * bc - br;
    return std::sqrt(gx * gx + gy * gy) * 0.25f; // normalize to [0,1]
}

void edgeDetect(const ConstImageView& src, ImageView dst, float thr,
                const ExecutionPolicy& policy) {
    const uint32 w = src.width(), h = src.height();
    const uint32 width = w;
    parallelForRows(h, [&](size_t row) {
        const uint32 y = static_cast<uint32>(row);
        for (uint32 x = 0; x < width; ++x) {
            float m = sobelMagnitude(src, static_cast<int32>(x), static_cast<int32>(y));
            float v = (m >= thr) ? 1.f : m; // keep gradient below threshold
            proc::detail::writeNorm(dst, x, y, 0, v);
            proc::detail::writeNorm(dst, x, y, 1, v);
            proc::detail::writeNorm(dst, x, y, 2, v);
        }
    }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: edge_demo <input-image> <output-image> [angle-threshold-degrees] [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outPath = argv[2];
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Edge detection (Sobel)", src);

    // Work on a single-channel grayscale view for speed.
    Image gray(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);
    proc::color::toGray(src.view(), gray.view(), policy);

    const float thr = static_cast<float>(std::cos(30.0 * 3.14159265358979323846 / 180.0));
    std::printf("  threshold: %.2f, backend: %s\n",
                static_cast<double>(thr), iml_example::policyName(serial));
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();

    Image edge(fmt::rgb8, src.width(), src.height(), ColorSpace::SRGB);
    iml_example::timeAndReport("sobel magnitude f/ thresh", pixels, [&]() {
        edgeDetect(gray.view(), edge.view(), thr, policy);
    });

    iml_example::saveOrDie(edge, outPath);
    std::printf("  wrote %s\n", outPath.c_str());
    return 0;
}