// filter_edge.cpp - Sobel gradient magnitude (edge detection).
// Applies the 3x3 Sobel X/Y kernels (correlation orientation, conv::Kernel
// classes) over the three color channels, then emits the gradient magnitude as
// a grayscale image. Neighborhood operator: both paths gather the 3x3 window
// through sampleNorm, so scalar and SIMD outputs are identical.
//   filter_edge.exe assets/mario.bmp out_edge.png
#include "example_util.h"

#include <cmath>

using namespace iml;
using namespace iml_example;

namespace {

// 3x3 Sobel X and Y weights (row-major, correlation orientation).
const float kSobelX[9] = {-1.f, 0.f, 1.f, -2.f, 0.f, 2.f, -1.f, 0.f, 1.f};
const float kSobelY[9] = {-1.f, -2.f, -1.f, 0.f, 0.f, 0.f, 1.f, 2.f, 1.f};
const int32 kOffset[9][2] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {0, 0},
                             {1, 0},   {-1, 1}, {0, 1},  {1, 1}};

inline float sobelAt(const ConstImageView& src, int32 x, int32 y) {
    float gx = 0.f, gy = 0.f;
    for (uint32 c = 0; c < 3; ++c) {
        float ax = 0.f, ay = 0.f;
        for (int k = 0; k < 9; ++k) {
const float s =
                proc::conv::detail::sampleNorm(src, x + kOffset[k][0], y + kOffset[k][1], c,
                                         proc::conv::BorderMode::Mirror);
            ax += kSobelX[k] * s;
            ay += kSobelY[k] * s;
        }
        gx += ax;
        gy += ay;
    }
    gx /= 3.f;
    gy /= 3.f;
    return math::clamp01(std::sqrt(gx * gx + gy * gy));
}

} // namespace

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_edge.png");

    try {
        const Image src(a.input);
        Image dst(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);

        const auto scalar = [&] {
            for (int32 y = 0; y < (int32)src.height(); ++y)
                for (int32 x = 0; x < (int32)src.width(); ++x)
                    pixel::writeNorm(dst.view(), x, y, 0, sobelAt(src.view(), x, y));
        };
        const auto submit = [&](const ExecutionPolicy& policy) {
            parallelForRows(
                src.height(),
                [&](size_t row) {
                    for (size_t col = 0; col < src.width(); ++col)
                        pixel::writeNorm(dst.view(), (int32)col, (int32)row, 0,
                                         sobelAt(src.view(), (int32)col, (int32)row));
                },
                policy);
        };

        if (a.serial) {
            scalar();
        } else {
            const uint64 pixels = (uint64)src.width() * src.height();
            reportTrio("edge (sobel)", pixels, scalar,
                       [&] { submit(ExecutionPolicy::serial()); },
                       [&] { submit(ExecutionPolicy::parallel()); },
                       "scalar (1 thread)", "optimized (1 thread)", "parallel");
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