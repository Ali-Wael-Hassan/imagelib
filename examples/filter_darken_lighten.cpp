// filter_darken_lighten.cpp - brightness offset, point operator.
// Adds `amount` to every channel, clamped to [0,1]. Negative darkens,
// positive brightens. Run with an optional amount:
//   filter_darken_lighten.exe assets/mario.bmp out_dark.png -0.35
//   filter_darken_lighten.exe assets/mario.bmp out_bright.png 0.25
#include "example_util.h"

using namespace iml;
using namespace iml_example;

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_darken.png");

    float amount = paramFloat(a, 0, -0.3f);

    try {
        const Image src(a.input);
        Image dst(src.format(), src.width(), src.height(), src.colorSpace(), src.alphaMode());

        const auto block = [amount](const Pix4& p) -> Pix4 {
            const V4 d = splat(amount);
            return {simd::clamp01(p.r + d), simd::clamp01(p.g + d),
                    simd::clamp01(p.b + d), p.a};
        };
        const auto scalar = [amount](const Pixel& p) {
            return Pixel(math::clamp01(p.r + amount), math::clamp01(p.g + amount),
                         math::clamp01(p.b + amount), p.a);
        };

        if (a.serial) {
            mapScalar(src.view(), dst.view(), scalar);
        } else {
            reportPoint("darken/lighten", src.view(), dst.view(), scalar, block);
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