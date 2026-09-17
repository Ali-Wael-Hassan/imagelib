// filter_black_white.cpp - luminance threshold, point operator.
// Luma below the threshold becomes black (0), at or above becomes white (1).
// Run with an optional threshold:  filter_black_white.exe img out.png 0.45
#include "example_util.h"

using namespace iml;
using namespace iml_example;

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_black_white.png");

    float threshold = paramFloat(a, 0, 0.5f);

    try {
        const Image src(a.input);
        Image dst(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);

        const auto block = [threshold](const Pix4& p) -> Pix4 {
            const V4 luma = p.r * 0.2126f + p.g * 0.7152f + p.b * 0.0722f;
            const V4 tile = simd::broadcast<float, 4>(threshold);
            const V4 sel = simd::clamp01(simd::max(luma - tile, V4()) * 1e6f); // 0 or 1
            return {sel, sel, sel, splat(1.f)};
        };
        const auto scalar = [threshold](const Pixel& p) {
            const float l = p.r * 0.2126f + p.g * 0.7152f + p.b * 0.0722f;
            const float v = l <= threshold ? 0.f : 1.f;   // must tie with SIMD path
            return Pixel(v, v, v, 1.f);
        };

        if (a.serial) {
            mapScalar(src.view(), dst.view(), scalar);
        } else {
            reportPoint("black & white", src.view(), dst.view(), scalar, block);
        }
        if (!a.output.empty())
            dst.save(a.output);
        std::cout << "wrote " << a.output << '\n';
        return 0;
    } catch (const Error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}