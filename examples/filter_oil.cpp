// filter_oil.cpp - oil-painting effect (window mode histogram).
// Within a (2r+1)^2 window each output pixel wins by "majority vote": gather
// the window, bucket every pixel by its luminance into `levels` bins, count +
// accumulate the channels per bin, then emit the average color of the most
// populated bin. Statistical neighborhood operator; scalar and SIMD paths do
// the same votes per lane, so outputs are identical.
//   filter_oil.exe assets/mario.bmp out_oil.png [radius] [levels]
#include "example_util.h"

#include <algorithm>
#include <vector>

using namespace iml;
using namespace iml_example;

namespace {

inline float luma0(const Pixel& p) {
    return math::clamp01(0.2126f * p.r + 0.7152f * p.g + 0.0722f * p.b);
}

Pixel oilAt(const ConstImageView& src, int32 x, int32 y, int rad, int levels) {
    // Fixed-size stack votes (levels capped at 256): no heap traffic per pixel.
    int cnt[256] = {};
    float sr[256] = {}, sg[256] = {}, sb[256] = {};
    const int L = levels < 1 ? 1 : (levels > 256 ? 256 : levels);

    for (int dy = -rad; dy <= rad; ++dy)
        for (int dx = -rad; dx <= rad; ++dx) {
            const int32 sx = math::clamp(x + dx, 0, (int32)src.width() - 1);
            const int32 sy = math::clamp(y + dy, 0, (int32)src.height() - 1);
            const Pixel p = pixel::readPixel(src, sx, sy);
            const int b = std::min(L - 1, (int)(luma0(p) * L));
            cnt[b] += 1;
            sr[b] += p.r;
            sg[b] += p.g;
            sb[b] += p.b;
        }

    int best = 0;
    for (int b = 1; b < L; ++b)
        if (cnt[b] > cnt[best])
            best = b;
    const Pixel center = pixel::readPixel(src, x, y);
    if (cnt[best] == 0)
        return Pixel(0.f, 0.f, 0.f, center.a);
    return Pixel(sr[best] / (float)cnt[best], sg[best] / (float)cnt[best],
                 sb[best] / (float)cnt[best], center.a);
}

} // namespace

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_oil.png");

    const int radius = std::max(1, (int32)paramFloat(a, 0, 3.f));
    const int levels = std::max(4, (int32)paramFloat(a, 1, 32.f));

    try {
        const Image src(a.input);
        Image dst(src.format(), src.width(), src.height(), src.colorSpace(), src.alphaMode());

        const auto scalar = [&] {
            for (int32 y = 0; y < (int32)src.height(); ++y)
                for (int32 x = 0; x < (int32)src.width(); ++x)
                    pixel::writeRGBA(oilAt(src.view(), x, y, radius, levels), dst.view(), x, y);
        };
        const auto submit = [&](const ExecutionPolicy& policy) {
            parallelForRows(
                src.height(),
                [&](size_t row) {
                    for (size_t col = 0; col < src.width(); ++col)
                        pixel::writeRGBA(
                            oilAt(src.view(), (int32)col, (int32)row, radius, levels),
                            dst.view(), (int32)col, (int32)row);
                },
                policy);
        };

        if (a.serial) {
            scalar();
        } else {
            const uint64 pixels = (uint64)src.width() * src.height();
            reportTrio("oil painting", pixels, scalar,
                       [&] { submit(ExecutionPolicy::serial()); },
                       [&] { submit(ExecutionPolicy::parallel()); },
                       "scalar (1 thread)", "optimized (1 thread)", "parallel");
        }
        if (!a.output.empty())
            dst.save(a.output);
        std::cout << "wrote " << a.output << " (radius " << radius << ", " << levels
                  << " levels)\n";
        return 0;
    } catch (const Error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}