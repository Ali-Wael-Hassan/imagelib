// filter_grayscale.cpp - Rec.709 luminance, point operator.
// Uses the SIMD per-pixel mapper (iml_example::mapRowsSimd4) which processes
// 4 pixels at a time per row via simd::Float4 and auto-vectorizes the luma
// math. Try:  filter_grayscale.exe assets/mario.bmp out_gray.png
#include "example_util.h"

using namespace iml;
using namespace iml_example;

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_gray.png");
    try {
        const Image src(a.input);
        Image dst(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);

        const auto block = [](const Pix4& p) -> Pix4 {
            const V4 luma = p.r * 0.2126f + p.g * 0.7152f + p.b * 0.0722f;
            return {luma, luma, luma, splat(1.f)};
        };
        const auto scalar = [](const Pixel& p) {
            const float l = p.r * 0.2126f + p.g * 0.7152f + p.b * 0.0722f;
            return Pixel(l, l, l, 1.f);
        };

        if (a.serial) {
            mapScalar(src.view(), dst.view(), scalar);
        } else {
            reportPoint("grayscale", src.view(), dst.view(), scalar, block);
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