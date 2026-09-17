// filter_blur.cpp - separable Gaussian blur.
// Two cache-friendly passes executed row-by-row: a horizontal pass (rows, in
// x-order) writes the intermediate, then a vertical pass (columns across rows)
// writes the destination. The hot loop is templated on the storage type and
// channel count so it reads/writes the sample memory directly - no per-sample
// type dispatch, no bounds checks, no function calls. Mirror border handling is
// only needed at the image edges, so the interior of every row takes the fast
// unguarded path.
//   filter_blur.exe assets/mario.bmp out_blur.png [radius]
#include "example_util.h"

#include <cmath>
#include <cstdint>
#include <vector>

using namespace iml;
using namespace iml_example;

namespace {

std::vector<float> gaussianWeights(int rad) {
    const float sigma = std::max(0.5f, static_cast<float>(rad) * 0.5f);
    std::vector<float> w(2 * rad + 1);
    float sum = 0.f;
    for (int k = -rad; k <= rad; ++k) {
        const float v = std::exp(-(k * k) / (2.f * sigma * sigma));
        w[k + rad] = v;
        sum += v;
    }
    for (float& v : w)
        v /= sum;
    return w;
}

/// Mirror index matches conv::detail::reflectIndex so the output is identical to
/// the generic path (Mirror border).
inline int32 mirr(int32 i, int32 n) noexcept {
    while (i < 0 || i >= n) {
        if (i < 0)
            i = -i - 1;
        else
            i = 2 * n - i - 1;
    }
    return i;
}

/// One separable pass over the sample memory of `src` into `dst`.
/// Horizontal reads samples along the row (unit stride); vertical reads the
/// same column of the neighbouring rows. `policy` selects serial or pooled
/// row execution. Normalized conversion goes through iml_example::NormConv
/// (matching pixel::readNorm / pixel::writeNorm exactly).
/// @tparam T Storage sample type (uint8/uint16/uint32/float).
/// @tparam Ch Channels per pixel (1..4).
template <typename T, uint32 Ch>
void blurPassTC(
    const ConstImageView& src,
    ImageView dst,
    const std::vector<float>& w,
    int rad,
    bool horizontal,
    const ExecutionPolicy& policy) {
    using C = NormConv<T>;
    const int32 W = (int32)src.width(), H = (int32)src.height();
    const byte* sBase = src.data();
    byte* dBase = dst.data();
    const size_t sStride = src.stride(), dStride = dst.stride();
    const float* wg = w.data();

    parallelForRows(
        (size_t)H,
        [&](size_t r) {
            const int32 y = (int32)r;
            const T* srow = reinterpret_cast<const T*>(sBase + (size_t)y * sStride);
            T* drow = reinterpret_cast<T*>(dBase + (size_t)y * dStride);
            if (horizontal) {
                for (int32 x = 0; x < W; ++x) {
                    float acc[Ch] = {};
                    for (int k = -rad; k <= rad; ++k) {
                        int32 sx = x + k;
                        if ((uint32)sx >= (uint32)W)
                            sx = mirr(sx, W);
                        const T* sp = srow + (size_t)sx * Ch;
                        const float wgt = wg[k + rad];
                        for (uint32 c = 0; c < Ch; ++c)
                            acc[c] += wgt * ((float)sp[c] * C::invScale);
                    }
                    T* dp = drow + (size_t)x * Ch;
                    for (uint32 c = 0; c < Ch; ++c)
                        dp[c] = C::quant(acc[c]);
                }
            } else {
                for (int32 x = 0; x < W; ++x) {
                    float acc[Ch] = {};
                    for (int k = -rad; k <= rad; ++k) {
                        int32 sy = y + k;
                        if ((uint32)sy >= (uint32)H)
                            sy = mirr(sy, H);
                        const T* sp =
                            reinterpret_cast<const T*>(sBase + (size_t)sy * sStride) +
                            (size_t)x * Ch;
                        const float wgt = wg[k + rad];
                        for (uint32 c = 0; c < Ch; ++c)
                            acc[c] += wgt * ((float)sp[c] * C::invScale);
                    }
                    T* dp = drow + (size_t)x * Ch;
                    for (uint32 c = 0; c < Ch; ++c)
                        dp[c] = C::quant(acc[c]);
                }
            }
        },
        policy);
}

/// Dispatches blurPassTC once per (data type, channel count) pair so the hot
/// loops never re-dispatch on format at sample granularity.
void blurPass(
    const ConstImageView& src,
    ImageView dst,
    const std::vector<float>& w,
    int rad,
    bool horizontal,
    const ExecutionPolicy& policy) {
    const uint32 ch = src.channels();
    switch (src.dataType()) {
    case DataType::UInt8:
        switch (ch) {
        case 1: return blurPassTC<uint8, 1>(src, dst, w, rad, horizontal, policy);
        case 2: return blurPassTC<uint8, 2>(src, dst, w, rad, horizontal, policy);
        case 3: return blurPassTC<uint8, 3>(src, dst, w, rad, horizontal, policy);
        case 4: return blurPassTC<uint8, 4>(src, dst, w, rad, horizontal, policy);
        default: break;
        }
        break;
    case DataType::UInt16:
        switch (ch) {
        case 1: return blurPassTC<uint16, 1>(src, dst, w, rad, horizontal, policy);
        case 2: return blurPassTC<uint16, 2>(src, dst, w, rad, horizontal, policy);
        case 3: return blurPassTC<uint16, 3>(src, dst, w, rad, horizontal, policy);
        case 4: return blurPassTC<uint16, 4>(src, dst, w, rad, horizontal, policy);
        default: break;
        }
        break;
    case DataType::UInt32:
        switch (ch) {
        case 1: return blurPassTC<uint32, 1>(src, dst, w, rad, horizontal, policy);
        case 2: return blurPassTC<uint32, 2>(src, dst, w, rad, horizontal, policy);
        case 3: return blurPassTC<uint32, 3>(src, dst, w, rad, horizontal, policy);
        case 4: return blurPassTC<uint32, 4>(src, dst, w, rad, horizontal, policy);
        default: break;
        }
        break;
    case DataType::Float32:
        switch (ch) {
        case 1: return blurPassTC<float, 1>(src, dst, w, rad, horizontal, policy);
        case 2: return blurPassTC<float, 2>(src, dst, w, rad, horizontal, policy);
        case 3: return blurPassTC<float, 3>(src, dst, w, rad, horizontal, policy);
        case 4: return blurPassTC<float, 4>(src, dst, w, rad, horizontal, policy);
        default: break;
        }
        break;
    default:
        break;
    }
    throw UnsupportedFormatError("blur: unsupported format");
}

} // namespace

int main(int argc, char** argv) {
    const RunArgs a = parse(argc, argv, "out_blur.png");

    const int radius = std::max(1, (int32)paramFloat(a, 0, 2.f));
    const std::vector<float> weights = gaussianWeights(radius);

    try {
        const Image src(a.input);
        Image tmp(src.format(), src.width(), src.height(), src.colorSpace(), src.alphaMode());
        Image dst(src.format(), src.width(), src.height(), src.colorSpace(), src.alphaMode());

        const auto scalar = [&] {
            blurPass(src.view(), tmp.view(), weights, radius, true, ExecutionPolicy::serial());
            blurPass(tmp.view(), dst.view(), weights, radius, false, ExecutionPolicy::serial());
        };
        const auto submit = [&](const ExecutionPolicy& policy) {
            blurPass(src.view(), tmp.view(), weights, radius, true, policy);
            blurPass(tmp.view(), dst.view(), weights, radius, false, policy);
        };

        if (a.serial) {
            scalar();
        } else {
            const uint64 pixels = (uint64)src.width() * src.height();
            reportTrio("blur (gaussian)", pixels, scalar,
                       [&] { submit(ExecutionPolicy::serial()); },
                       [&] { submit(ExecutionPolicy::parallel()); },
                       "scalar (1 thread)", "optimized (1 thread)", "parallel");
        }
        if (!a.output.empty())
            dst.save(a.output);
        std::cout << "wrote " << a.output << " (radius " << radius << ")\n";
        return 0;
    } catch (const Error& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}