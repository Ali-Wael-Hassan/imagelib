// examples/rotate_demo.cpp
//
// Example 6: Rotate image by an arbitrary angle (degrees, default 90).
// Implemented as a custom bilinear-sampling kernel that runs over the
// row-parallel partition. Each destination pixel inverse-maps into the source
// and bilinearly interpolates four neighbours.
//
// Usage: rotate_demo <input-image> <output-image> [degrees] [serial]

#include "example_util.h"

using namespace iml;

namespace {

const float kPi = 3.14159265358979323846f;

// Bilinear + clamp sample.
inline float bilerp(const ConstImageView& src, float fx, float fy, uint32 c) {
    const int32 w = static_cast<int32>(src.width());
    const int32 h = static_cast<int32>(src.height());
    const int32 x0 = std::max<int32>(0, std::min<int32>(w - 1, static_cast<int32>(fx)));
    const int32 y0 = std::max<int32>(0, std::min<int32>(h - 1, static_cast<int32>(fy)));
    const int32 x1 = std::max<int32>(0, std::min<int32>(w - 1, x0 + 1));
    const int32 y1 = std::max<int32>(0, std::min<int32>(h - 1, y0 + 1));
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);
    const float v00 = proc::detail::readNorm(src, x0, y0, c);
    const float v10 = proc::detail::readNorm(src, x1, y0, c);
    const float v01 = proc::detail::readNorm(src, x0, y1, c);
    const float v11 = proc::detail::readNorm(src, x1, y1, c);
    const float top = v00 + (v10 - v00) * tx;
    const float bot = v01 + (v11 - v01) * tx;
    return top + (bot - top) * ty;
}

// Rotates `src` by `rad` about the image centre, writing a same-size canvas.
void rotate(const ConstImageView& src, ImageView dst, float rad,
            const ExecutionPolicy& policy) {
    const float c = std::cos(rad), s = std::sin(rad);
    const float cx = 0.5f * static_cast<float>(src.width() - 1);
    const float cy = 0.5f * static_cast<float>(src.height() - 1);
    const uint32 ch = std::min<uint32>(src.channels(), dst.channels());
    const uint32 w = dst.width(), h = dst.height();
    const uint32 width = w;
    parallelForRows(h, [&](size_t row) {
        const float y = static_cast<float>(row) - cy;
        for (uint32 x = 0; x < width; ++x) {
            const float dx = static_cast<float>(x) - cx;
            // Inverse rotation: src = R^-1 * (dst - centre):
            //   sx =  c*dx + s*y ;  sy = -s*dx + c*y
            const float sx = c * dx + s * y + cx;
            const float sy = -s * dx + c * y + cy;
            for (uint32 cc = 0; cc < ch; ++cc)
                proc::detail::writeNorm(dst, x, static_cast<uint32>(row), cc,
                                       bilerp(src, sx, sy, cc));
        }
    }, policy);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("Usage: rotate_demo <input-image> <output-image> [degrees] [serial]\n");
        return 1;
    }
    const std::string inPath = argv[1], outPath = argv[2];
    const float degrees = argc >= 4 ? static_cast<float>(std::atof(argv[3])) : 90.0f;
    const bool serial = iml_example::wantsSerial(argc, argv);
    const ExecutionPolicy policy = iml_example::defaultPolicy(serial);

    Image src = iml_example::loadOrDie(inPath);
    iml_example::banner("Rotate image", src);
    std::printf("  backend: %s, angle: %.1f deg\n",
                iml_example::policyName(serial), static_cast<double>(degrees));
    const uint64 pixels = static_cast<uint64>(src.width()) * src.height();

    Image dst(fmt::rgb8, src.width(), src.height(), ColorSpace::SRGB);
    iml_example::timeAndReport("rotate (bilinear)", pixels, [&]() {
        rotate(src.view(), dst.view(), degrees * kPi / 180.0f, policy);
    });

    iml_example::saveOrDie(dst, outPath);
    std::printf("  wrote %s\n", outPath.c_str());
    return 0;
}