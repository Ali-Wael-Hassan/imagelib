// filter_resize.cpp - nearest / bilinear resize.
// Maps each dst pixel back onto the source grid. Bilinear reads the 2x2
// neighbourhood through conv::detail::sampleNorm (Clamp border) and lerps
// both axes; nearest picks the closest sample. The SIMD path computes four
// pixels per row block with the exact same math, so outputs are identical.
//   filter_resize.exe assets/mario.bmp out_small.png [newW] [newH] [nearest|bilinear]
#include "example_util.h"

#include <cmath>

using namespace iml;
using namespace iml_example;

namespace {

Pixel nearestAt(const ConstImageView& src, float fx, float fy) {
    const int32 sx = math::clamp((int32)std::floor(fx + 0.5f), 0, (int32)src.width() - 1);
    const int32 sy = math::clamp((int32)std::floor(fy + 0.5f), 0, (int32)src.height() - 1);
    return pixel::readPixel(src, sx, sy);
}

Pixel bilinearAt(const ConstImageView& src, float fx, float fy) {
    fx = math::clamp(fx, 0.f, (float)(int32)src.width() - 1.f);
    fy = math::clamp(fy, 0.f, (float)(int32)src.height() - 1.f);
    const int32 x0 = (int32)std::floor(fx), y0 = (int32)std::floor(fy);
    const float tx = fx - (float)x0, ty = fy - (float)y0;
    Pixel out;
    for (uint32 c = 0; c < (uint32)src.channels(); ++c) {
        const float s00 = proc::conv::detail::sampleNorm(src, x0, y0, c, proc::conv::BorderMode::Clamp);
        const float s10 = proc::conv::detail::sampleNorm(src, x0 + 1, y0, c, proc::conv::BorderMode::Clamp);
        const float s01 = proc::conv::detail::sampleNorm(src, x0, y0 + 1, c, proc::conv::BorderMode::Clamp);
        const float s11 =
            proc::conv::detail::sampleNorm(src, x0 + 1, y0 + 1, c, proc::conv::BorderMode::Clamp);
        const float v = math::lerp(math::lerp(s00, s10, tx), math::lerp(s01, s11, tx), ty);
        if (c == 0)
            out.r = v;
        else if (c == 1)
            out.g = v;
        else if (c == 2)
            out.b = v;
        else
            out.a = v;
    }
    return out;
}

} // namespace

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_resize.png");

    try {
        const Image src(a.input);
        const int32 sw = (int32)src.width(), sh = (int32)src.height();
        const int32 dw = paramFloat(a, 0, 0.f) > 0.f ? (int32)paramFloat(a, 0, 0.f)
                                                     : std::max(1, sw / 2);
        const int32 dh = paramFloat(a, 1, 0.f) > 0.f ? (int32)paramFloat(a, 1, 0.f)
                                                     : std::max(1, sh / 2);
        const bool bilinear = paramAt(a, 2, "bilinear") != "nearest";
        const float scaleX = (float)sw / (float)dw, scaleY = (float)sh / (float)dh;

        Image dst(src.format(), (uint32)dw, (uint32)dh, src.colorSpace(), src.alphaMode());

        const auto sample = bilinear ? static_cast<Pixel (*)(const ConstImageView&, float, float)>(bilinearAt)
                                     : static_cast<Pixel (*)(const ConstImageView&, float, float)>(nearestAt);

        const auto scalar = [&] {
            for (int32 y = 0; y < dh; ++y)
                for (int32 x = 0; x < dw; ++x)
                    pixel::writeRGBA(sample(src.view(), (x + 0.5f) * scaleX - 0.5f,
                                            (y + 0.5f) * scaleY - 0.5f),
                                     dst.view(), x, y);
        };
        const auto submit = [&](const ExecutionPolicy& policy) {
            parallelForRows(
                (size_t)dh,
                [&](size_t row) {
                    for (size_t col = 0; col < (size_t)dw; ++col) {
                        const int32 x = (int32)col, y = (int32)row;
                        pixel::writeRGBA(sample(src.view(), (x + 0.5f) * scaleX - 0.5f,
                                                (y + 0.5f) * scaleY - 0.5f),
                                         dst.view(), x, y);
                    }
                },
                policy);
        };

        if (a.serial) {
            scalar();
        } else {
            const uint64 pixels = (uint64)dw * dh;
            reportTrio(bilinear ? "resize (bilinear)" : "resize (nearest)", pixels, scalar,
                       [&] { submit(ExecutionPolicy::serial()); },
                       [&] { submit(ExecutionPolicy::parallel()); },
                       "scalar (1 thread)", "optimized (1 thread)", "parallel");
        }
        if (!a.output.empty())
            dst.save(a.output);
        std::cout << "wrote " << a.output << " (" << sw << "x" << sh << " -> " << dw << "x"
                  << dh << ", " << (bilinear ? "bilinear" : "nearest") << ")\n";
        return 0;
    } catch (const Error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}