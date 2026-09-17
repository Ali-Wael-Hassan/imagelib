// filter_rotate.cpp - rotate 90 / 180 / 270 degrees (no resampling).
// A pure remap: every output pixel (x, y) reads one source pixel. 90 and 270
// produce a swapped-size image; the SIMD blocks gather 4 source pixels per dst
// column-block (vertically along a source column, no resampling involved).
//   filter_rotate.exe assets/mario.bmp out_r180.png 180
#include "example_util.h"

#include <string>

using namespace iml;
using namespace iml_example;

namespace iml_example {

// dst(x, y) <- src(srcX, srcY) for the library's y-down coordinate system.
inline int32 srcX(int32 x, int32 y, int angle, int32 w, int32 h) {
    switch (angle) {
    case 180: return w - 1 - x;
    case 270: return w - 1 - y;
    case 90:
    default: return y;
    }
}

inline int32 srcY(int32 x, int32 y, int angle, int32 w, int32 h) {
    switch (angle) {
    case 180: return h - 1 - y;
    case 270: return x;
    case 90:
    default: return h - 1 - x;
    }
}

} // namespace iml_example

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_rotate.png");

    int angle = static_cast<int>(paramFloat(a, 0, 90.f));
    if (angle != 90 && angle != 180 && angle != 270) {
        std::cerr << "angle must be 90, 180 or 270\n";
        return 1;
    }

    try {
        const Image src(a.input);
        const int32 sw = static_cast<int32>(src.width()), sh = static_cast<int32>(src.height());
        const uint32 dw = (angle == 180) ? src.width() : src.height();
        const uint32 dh = (angle == 180) ? src.height() : src.width();
        Image dst(src.format(), dw, dh, src.colorSpace(), src.alphaMode());

        const auto scalar = [&] {
            for (int32 y = 0; y < static_cast<int32>(dh); ++y)
                for (int32 x = 0; x < static_cast<int32>(dw); ++x)
                    pixel::writeRGBA(pixel::readPixel(src.view(), srcX(x, y, angle, sw, sh),
                                                      srcY(x, y, angle, sw, sh)),
                                     dst.view(), x, y);
        };

        const auto tail = [&](size_t row, size_t col) {
            const int32 x = static_cast<int32>(col), y = static_cast<int32>(row);
            pixel::writeRGBA(pixel::readPixel(src.view(), srcX(x, y, angle, sw, sh),
                                              srcY(x, y, angle, sw, sh)),
                             dst.view(), x, y);
        };

        const auto submit = [&](const ExecutionPolicy& policy) {
            if (angle == 180) {
                parallelForRowsSimd<V4>(
                    dw, dh,
                    [&](size_t row, size_t colStart) {
                        const size_t sy = sh - 1 - row;
                        Pix4 p = loadRow4(src.view(), sw - 4 - static_cast<int32>(colStart),
                                          static_cast<int32>(sy));
                        storeRow4(reversePix4(p), dst.view(), static_cast<int32>(colStart),
                                  static_cast<int32>(row));
                    },
                    tail, policy);
            } else if (angle == 90) {
                // dst row y reads source column y; source rows come from the
                // dst columns in reverse, so gather them then lane-reverse.
                parallelForRowsSimd<V4>(
                    dw, dh,
                    [&](size_t row, size_t colStart) {
                        const int32 sx = static_cast<int32>(row);
                        float r[4], g[4], b[4], a[4];
                        for (int i = 0; i < 4; ++i) {
                            const Pixel p = pixel::readPixel(src.view(), sx,
                                                             sh - 1 - (static_cast<int32>(colStart) + i));
                            r[i] = p.r;
                            g[i] = p.g;
                            b[i] = p.b;
                            a[i] = p.a;
                        }
                        storeRow4({simd::load<float, 4>(r), simd::load<float, 4>(g),
                                   simd::load<float, 4>(b), simd::load<float, 4>(a)},
                                  dst.view(), static_cast<int32>(colStart), static_cast<int32>(row));
                    },
                    tail, policy);
            } else { // 270
                // dst row y reads source column sw-1-y; source rows = dst columns.
                parallelForRowsSimd<V4>(
                    dw, dh,
                    [&](size_t row, size_t colStart) {
                        const int32 sx = sw - 1 - static_cast<int32>(row);
                        float r[4], g[4], b[4], a[4];
                        for (int i = 0; i < 4; ++i) {
                            const Pixel p = pixel::readPixel(src.view(), sx,
                                                             static_cast<int32>(colStart) + i);
                            r[i] = p.r;
                            g[i] = p.g;
                            b[i] = p.b;
                            a[i] = p.a;
                        }
                        storeRow4({simd::load<float, 4>(r), simd::load<float, 4>(g),
                                   simd::load<float, 4>(b), simd::load<float, 4>(a)},
                                  dst.view(), static_cast<int32>(colStart), static_cast<int32>(row));
                    },
                    tail, policy);
            }
        };

        if (a.serial) {
            scalar();
        } else {
            const uint64 pixels = (uint64)dw * dh;
            reportTrio("rotate", pixels, scalar, [&] { submit(ExecutionPolicy::simd()); },
                       [&] { submit(ExecutionPolicy::simdParallel()); });
        }
        if (!a.output.empty())
            dst.save(a.output);
        std::cout << "wrote " << a.output << " (" << angle << " deg, "
                  << dw << "x" << dh << ")\n";
        return 0;
    } catch (const Error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}