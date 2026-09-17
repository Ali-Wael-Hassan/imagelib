// filter_invert.cpp - 1 - pixel per channel, point operator.
// The SIMD block maps four pixels per row in one vectorized pass over RGBA.
// Try:  filter_invert.exe assets/mario.bmp out_inv.png
#include "example_util.h"

using namespace iml;
using namespace iml_example;

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_invert.png");
    try {
        const Image src(a.input);
        Image dst(src.format(), src.width(), src.height(), src.colorSpace(), src.alphaMode());

        const auto block = [](const Pix4& p) -> Pix4 {
            return {splat(1.f) - p.r, splat(1.f) - p.g, splat(1.f) - p.b, p.a};
        };
        const auto scalar = [](const Pixel& p) { return Pixel(1.f - p.r, 1.f - p.g, 1.f - p.b, p.a); };

        if (a.serial) {
            mapScalar(src.view(), dst.view(), scalar);
        } else {
            reportPoint("invert", src.view(), dst.view(), scalar, block);
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