#pragma once
#ifndef IMAGELIB_BENCHMARK_PAIRENTRY_H_
#define IMAGELIB_BENCHMARK_PAIRENTRY_H_

/// @file imagelib/benchmark/record/PairEntry.h
/// A serial-vs-parallel measurement pair stored as one report row.

#include "imagelib/core/Types.h"

#include <string>

namespace iml {
namespace bench {

/// A serial-vs-parallel measurement of the same kernel (one table row).
struct PairEntry {
    /// Name of the benchmarked kernel.
    std::string name;
    uint64 pixels = 0; ///< pixel count used for both throughputs
    /// Measured serial run time in milliseconds.
    double msSerial = 0.0;
    /// Measured parallel run time in milliseconds.
    double msParallel = 0.0;
    double cut = 0.0; ///< optional budget/limit in ms (0 = none)

    /// Serial throughput in Mega-pixels/second.
    /// @return Mega-pixels processed per second.
    double mpixSerial() const noexcept;
    /// Parallel throughput in Mega-pixels/second.
    /// @return Mega-pixels processed per second.
    double mpixParallel() const noexcept;
    /// Parallel speedup over the serial baseline (1.0 = no gain).
    /// @return Serial time divided by parallel time, else 1.0.
    double speedup() const noexcept;
};

} // namespace bench
} // namespace iml

#endif