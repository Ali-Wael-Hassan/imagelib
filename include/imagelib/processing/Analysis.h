#pragma once
#define IMAGELIB_PROCESSING_ANALYSIS_H_
// imagelib/processing/Analysis.h
//
// Image statistics and histograms: per-channel min/max/mean/std, intensity
// histograms and Otsu thresholding on channel 0.

#include "imagelib/processing/Pixel.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace iml {
namespace proc {
namespace analysis {

/// Number of bins used for 8-bit histograms.
constexpr size_t histogramBins8 = 256;

/// Per-channel intensity histogram of channel `c` (values quantized into
/// `bins` evenly spaced buckets over [0, 1]).
std::vector<uint64> histogram(const ConstImageView& src, uint32 c, size_t bins);

uint64 histogramPixels(const ConstImageView& src) noexcept;

/// Min and max of channel `c` (no bounds inserted; Zero border corners are
/// naturally the samples themselves).
void minMax(const ConstImageView& src, uint32 c, float& outMin, float& outMax);

/// Mean and (population) standard deviation of channel `c`.
void meanStdDev(const ConstImageView& src, uint32 c, float& outMean, float& outStd);

/// Mean of channel `c`.
float mean(const ConstImageView& src, uint32 c);

/// Otsu's auto-threshold on channel 0 using a `bins`-bin histogram of [0,1].
/// Returns the threshold value in [0, 1] that maximizes inter-class variance.
float otsuThreshold(const ConstImageView& src, size_t bins = histogramBins8);

/// Shannon entropy (bits) of the channel-0 histogram.
float entropyBits(const ConstImageView& src, size_t bins = histogramBins8);

} // namespace analysis
} // namespace proc
} // namespace iml