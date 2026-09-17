// filter_merge.cpp - alpha blend of two images.
// out = a * t + b * (1 - t), with t = opacity * a.alpha. The SIMD path blends
// four pixels per row in one vectorized pass over two sources.
//   filter_merge.exe first.png second.png out.png [opacity]
#include "example_util.h"

using namespace iml;
using namespace iml_example;

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_merge.png");

    const std::string secondPath = pickInput(paramAt(a, 0, "assets/photographer.bmp"));
    const float opacity = paramFloat(a, 1, 0.5f);

    if (opacity < 0.f || opacity > 1.f) {
        std::cerr << "opacity must be in [0, 1]\n";
        return 1;
    }

    try {
        const Image srcA(a.input);
        const Image srcB(secondPath);
        if (srcA.width() != srcB.width() || srcA.height() != srcB.height()) {
            std::cerr << "images must have identical dimensions\n";
            return 1;
        }
        Image dst(srcA.format(), srcA.width(), srcA.height(), srcA.colorSpace(), srcA.alphaMode());

        const auto block = [opacity](const Pix4& x, const Pix4& y) -> Pix4 {
            const V4 t = splat(opacity) * x.a;   // opacity * alpha of the front image
            const V4 inv = splat(1.f) - t;
            return {x.r * t + y.r * inv, x.g * t + y.g * inv, x.b * t + y.b * inv, splat(1.f)};
        };
        const auto scalar = [opacity](const Pixel& x, const Pixel& y) {
            const float t = opacity * x.a;
            return Pixel(x.r * t + y.r * (1.f - t), x.g * t + y.g * (1.f - t),
                         x.b * t + y.b * (1.f - t), 1.f);
        };
        const auto mapScalarPair = [&](const Pixel& x, const Pixel& y) { return scalar(x, y); };

        if (a.serial) {
            const int32 w = (int32)srcA.width(), h = (int32)srcA.height();
            for (int32 y = 0; y < h; ++y)
                for (int32 x = 0; x < w; ++x) {
                    const Pixel A = pixel::readPixel(srcA.view(), x, y);
                    const Pixel B = pixel::readPixel(srcB.view(), x, y);
                    pixel::writeRGBA(mapScalarPair(A, B), dst.view(), x, y);
                }
        } else {
            const uint64 pixels = (uint64)srcA.width() * srcA.height();
            reportTrio(
                "merge", pixels,
                [&] {
                    const int32 w = (int32)srcA.width(), h = (int32)srcA.height();
                    for (int32 y = 0; y < h; ++y)
                        for (int32 x = 0; x < w; ++x) {
                            const Pixel B = pixel::readPixel(srcB.view(), x, y);
                            pixel::writeRGBA(
                                mapScalarPair(pixel::readPixel(srcA.view(), x, y), B),
                                dst.view(), x, y);
                        }
                },
                [&] { mapRowsSimd4(srcA.view(), srcB.view(), dst.view(), block, ExecutionPolicy::simd()); },
                [&] { mapRowsSimd4(srcA.view(), srcB.view(), dst.view(), block, ExecutionPolicy::simdParallel()); },
                "scalar (1 thread)", "optimized (1 thread)", "optimized + parallel");
        }

        if (!a.output.empty())
            dst.save(a.output);
        std::cout << "wrote " << a.output << '\n';
        std::cout << "blended " << srcA.width() << "x" << srcA.height() << " at opacity " << opacity << '\n';
        return 0;
    } catch (const Error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}