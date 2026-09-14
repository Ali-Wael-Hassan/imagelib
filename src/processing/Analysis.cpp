#include "imagelib/processing/Analysis.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace iml {
namespace proc {
namespace analysis {

std::vector<uint64> histogram(const ConstImageView& src, uint32 c, size_t bins) {
    if (!src.valid()) throw InvalidParameterError("analysis::histogram: invalid view");
    if (c >= src.channels()) throw InvalidParameterError("analysis::histogram: channel out of range");
    if (bins == 0) throw InvalidParameterError("analysis::histogram: bins == 0");
    std::vector<uint64> h(bins, 0);
    const uint32 w = src.width(), hh = src.height();
    for (uint32 y = 0; y < hh; ++y) {
        for (uint32 x = 0; x < w; ++x) {
            const float v = detail::readNorm(src, x, y, c);
            size_t bin = static_cast<size_t>(v * static_cast<float>(bins));
            if (bin >= bins) bin = bins - 1;
            ++h[bin];
        }
    }
    return h;
}

uint64 histogramPixels(const ConstImageView& src) noexcept {
    return static_cast<uint64>(src.width()) * src.height();
}

void minMax(const ConstImageView& src, uint32 c, float& outMin, float& outMax) {
    if (!src.valid()) throw InvalidParameterError("analysis::minMax: invalid view");
    if (c >= src.channels()) throw InvalidParameterError("analysis::minMax: channel out of range");
    const uint32 w = src.width(), h = src.height();
    outMin = 1.f, outMax = 0.f;
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x) {
            const float v = detail::readNorm(src, x, y, c);
            if (v < outMin) outMin = v;
            if (v > outMax) outMax = v;
        }
}

void meanStdDev(const ConstImageView& src, uint32 c, float& outMean, float& outStd) {
    if (!src.valid()) throw InvalidParameterError("analysis::meanStdDev: invalid view");
    if (c >= src.channels()) throw InvalidParameterError("analysis::meanStdDev: channel out of range");
    const uint64 n = histogramPixels(src);
    const uint32 w = src.width(), h = src.height();
    double sum = 0.0, sumsq = 0.0;
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x) {
            const double v = detail::readNorm(src, x, y, c);
            sum += v;
            sumsq += v * v;
        }
    const double mean = n == 0 ? 0.0 : sum / static_cast<double>(n);
    const double var  = n == 0 ? 0.0 : (sumsq / static_cast<double>(n)) - mean * mean;
    outMean = static_cast<float>(mean);
    outStd  = static_cast<float>(var > 0.0 ? std::sqrt(var) : 0.0);
}

float mean(const ConstImageView& src, uint32 c) {
    float m = 0.f, s = 0.f;
    meanStdDev(src, c, m, s);
    return m;
}

float otsuThreshold(const ConstImageView& src, size_t bins) {
    if (!src.valid() || bins == 0) throw InvalidParameterError("analysis::otsuThreshold: invalid input");
    const std::vector<uint64> h = histogram(src, 0, bins);
    const uint64 total = histogramPixels(src);
    if (total == 0) return 0.f;

    double sumB = 0.0; // weighted sum of background intensities
    for (size_t i = 0; i < bins; ++i) sumB += static_cast<double>(i + 1) * h[i];

    double wB = 0.0, sumWB = 0.0;
    double best = -1.0;
    size_t bestT = 0;
    for (size_t t = 0; t < bins; ++t) {
        wB += static_cast<double>(h[t]);
        if (wB == 0.0) continue;
        const double wF = static_cast<double>(total) - wB;
        if (wF == 0.0) break;
        sumWB += static_cast<double>(t + 1) * h[t];
        const double mB = sumWB / wB;
        const double mF = (sumB - sumWB) / wF;
        const double between = wB * wF * (mB - mF) * (mB - mF);
        if (between > best) { best = between; bestT = t; }
    }
    return (static_cast<float>(bestT) + 0.5f) / static_cast<float>(bins);
}

float entropyBits(const ConstImageView& src, size_t bins) {
    if (!src.valid() || bins == 0) throw InvalidParameterError("analysis::entropyBits: invalid input");
    const std::vector<uint64> h = histogram(src, 0, bins);
    const uint64 n = histogramPixels(src);
    if (n == 0) return 0.f;
    double e = 0.0;
    for (uint64 c : h) {
        if (c == 0) continue;
        const double p = static_cast<double>(c) / static_cast<double>(n);
        e -= p * std::log2(p);
    }
    return static_cast<float>(e);
}

} // namespace analysis
} // namespace proc
} // namespace iml