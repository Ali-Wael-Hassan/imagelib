// filter_flip.cpp - horizontal / vertical / both-flip of an image.
// A pure per-pixel remap: dst(x, y) reads src(sx, sy) where the axis decide
// whether sx and sy are mirrored. The SIMD path reverses a 4-pixel block with
// reversePix4 for the horizontal case, so scalar and SIMD stay identical.
//   filter_flip.exe assets/mario.bmp out_flip.png h|v|hv
#include "example_util.h"

#include <string>

using namespace iml;
using namespace iml_example;

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_flip.png");

    const std::string axis = paramAt(a, 0, "h");
    const bool hFlip = axis.find('h') != std::string::npos;
    const bool vFlip = axis.find('v') != std::string::npos;
    if (!hFlip && !vFlip) {
        std::cerr << "axis must be h, v or hv\n";
        return 1;
    }

    try {
        const Image src(a.input);
        Image dst(src.format(), src.width(), src.height(), src.colorSpace(), src.alphaMode());

        const size_t W = src.width(), H = src.height();

        const auto submit = [&](const ExecutionPolicy& policy) {
            parallelForRowsSimd<V4>(
                W,
                H,
                [&](size_t row, size_t colStart) {
                    const size_t sy = vFlip ? H - 1 - row : row;
                    const int32  c0 = static_cast<int32>(colStart);
                    if (hFlip) {
                        // dst block [c0, c0+4) map to src block [W-4-c0, W-1-c0],
                        // read in order then lane-reverse to land on dst columns.
                        Pix4 p = loadRow4(src.view(), static_cast<int32>(W) - 4 - c0, static_cast<int32>(sy));
                        storeRow4(reversePix4(p), dst.view(), c0, static_cast<int32>(row));
                    } else {
                        Pix4 p = loadRow4(src.view(), c0, static_cast<int32>(sy));
                        storeRow4(p, dst.view(), c0, static_cast<int32>(row));
                    }
                },
                [&](size_t row, size_t col) {
                    const int32 x = static_cast<int32>(col), y = static_cast<int32>(row);
                    const int32 sx = hFlip ? static_cast<int32>(W) - 1 - x : x;
                    const int32 sy = vFlip ? static_cast<int32>(H) - 1 - y : y;
                    pixel::writeRGBA(pixel::readPixel(src.view(), sx, sy), dst.view(), x, y);
                },
                policy);
        };
        const auto scalar = [&] {
            for (int32 y = 0; y < static_cast<int32>(H); ++y)
                for (int32 x = 0; x < static_cast<int32>(W); ++x) {
                    const int32 sx = hFlip ? static_cast<int32>(W) - 1 - x : x;
                    const int32 sy = vFlip ? static_cast<int32>(H) - 1 - y : y;
                    pixel::writeRGBA(pixel::readPixel(src.view(), sx, sy), dst.view(), x, y);
                }
        };

        if (a.serial) {
            scalar();
        } else {
            const uint64 pixels = (uint64)W * H;
            reportTrio("flip", pixels, scalar, [&] { submit(ExecutionPolicy::simd()); },
                       [&] { submit(ExecutionPolicy::simdParallel()); });
        }
        if (!a.output.empty())
            dst.save(a.output);
        std::cout << "wrote " << a.output << " (axis=\"" << axis << "\")\n";
        return 0;
    } catch (const Error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}