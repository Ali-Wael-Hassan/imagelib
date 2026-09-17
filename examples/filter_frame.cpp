// filter_frame.cpp - draw a colored border over a copy of the image.
// Border pixels (outside a thickness-px margin) are replaced by a solid color;
// interior pixels are copied as-is. A pure per-pixel point operator, so the
// SIMD path reuses the row block mapper with a coordinate-aware pick.
//   filter_frame.exe assets/mario.bmp out_frame.png [thickness] [r g b]
#include "example_util.h"

using namespace iml;
using namespace iml_example;

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_frame.png");

    const int32 thickness = std::max(1, (int32)paramFloat(a, 0, 4.f));
    const Pixel color(paramFloat(a, 1, 1.f), paramFloat(a, 2, 0.f), paramFloat(a, 3, 0.f));

    try {
        const Image src(a.input);
        const int32 W = (int32)src.width(), H = (int32)src.height();
        Image dst(src.format(), src.width(), src.height(), src.colorSpace(), src.alphaMode());

        const auto framePick = [&](int32 x, int32 y) {
            const bool edge = x < thickness || y < thickness || x >= W - thickness || y >= H - thickness;
            return edge ? color : pixel::readPixel(src.view(), x, y);
        };

        const auto scalar = [&] {
            for (int32 y = 0; y < H; ++y)
                for (int32 x = 0; x < W; ++x)
                    pixel::writeRGBA(framePick(x, y), dst.view(), x, y);
        };
        const auto submit = [&](const ExecutionPolicy& policy) {
            parallelForRows(
                (size_t)H,
                [&](size_t row) {
                    for (size_t col = 0; col < (size_t)W; ++col)
                        pixel::writeRGBA(framePick((int32)col, (int32)row), dst.view(),
                                         (int32)col, (int32)row);
                },
                policy);
        };

        if (a.serial) {
            scalar();
        } else {
            const uint64 pixels = (uint64)W * H;
            reportTrio("frame", pixels, scalar,
                       [&] { submit(ExecutionPolicy::serial()); },
                       [&] { submit(ExecutionPolicy::parallel()); },
                       "scalar (1 thread)", "optimized (1 thread)", "parallel");
        }
        if (!a.output.empty())
            dst.save(a.output);
        std::cout << "wrote " << a.output << " (thickness " << thickness << ")\n";
        return 0;
    } catch (const Error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}