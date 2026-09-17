// filter_crop.cpp - axis-aligned ROI crop.
// Copies a w x h rectangle starting at (x0, y0) into a fresh, smaller image.
// Rows stay contiguous, so the SIMD path copies 4-pixel blocks verbatim.
//   filter_crop.exe assets/mario.bmp out_crop.png [x0 y0 w h]
#include "example_util.h"

using namespace iml;
using namespace iml_example;

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_crop.png");

    try {
        const Image src(a.input);
        const int32 sw = static_cast<int32>(src.width()), sh = static_cast<int32>(src.height());

        // Default: the centered half of the image.
        int32 x0 = static_cast<int32>(paramFloat(a, 0, 0.f));
        int32 y0 = static_cast<int32>(paramFloat(a, 1, 0.f));
        int32 cw = static_cast<int32>(paramFloat(a, 2, (float)(sw / 2)));
        int32 ch = static_cast<int32>(paramFloat(a, 3, (float)(sh / 2)));
        if (a.params.empty()) { // centered half when no explicit rect was given
            x0 = sw / 4;
            y0 = sh / 4;
        }
        if (x0 < 0 || y0 < 0 || cw <= 0 || ch <= 0 || x0 + cw > sw || y0 + ch > sh) {
            std::cerr << "crop rect (" << x0 << ", " << y0 << ", " << cw << "x" << ch
                      << ") must fit inside " << sw << "x" << sh << '\n';
            return 1;
        }

        Image dst(src.format(), (uint32)cw, (uint32)ch, src.colorSpace(), src.alphaMode());

        const auto scalar = [&] {
            for (int32 y = 0; y < ch; ++y)
                for (int32 x = 0; x < cw; ++x)
                    pixel::writeRGBA(pixel::readPixel(src.view(), x0 + x, y0 + y), dst.view(), x, y);
        };
        const auto submit = [&](const ExecutionPolicy& policy) {
            parallelForRowsSimd<V4>(
                (size_t)cw, (size_t)ch,
                [&](size_t row, size_t colStart) {
                    const Pix4 p = loadRow4(src.view(), x0 + (int32)colStart, y0 + (int32)row);
                    storeRow4(p, dst.view(), (int32)colStart, (int32)row);
                },
                [&](size_t row, size_t col) {
                    pixel::writeRGBA(pixel::readPixel(src.view(), x0 + (int32)col, y0 + (int32)row),
                                     dst.view(), (int32)col, (int32)row);
                },
                policy);
        };

        if (a.serial) {
            scalar();
        } else {
            const uint64 pixels = (uint64)cw * ch;
            reportTrio("crop", pixels, scalar, [&] { submit(ExecutionPolicy::simd()); },
                       [&] { submit(ExecutionPolicy::simdParallel()); });
        }
        if (!a.output.empty())
            dst.save(a.output);
        std::cout << "wrote " << a.output << " (" << cw << "x" << ch << " at "
                  << x0 << ", " << y0 << ")\n";
        return 0;
    } catch (const Error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}