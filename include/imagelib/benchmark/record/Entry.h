#pragma once
#ifndef IMAGELIB_BENCHMARK_ENTRY_H_
#define IMAGELIB_BENCHMARK_ENTRY_H_

/// @file imagelib/benchmark/record/Entry.h
/// One measured benchmark entry.

#include "imagelib/core/Types.h"

#include <string>

namespace iml {
namespace bench {

/// One measured entry.
struct Entry {
    /// Name of the benchmarked kernel.
    std::string name;
    /// Pixel count processed by the benchmark.
    uint64 pixels = 0;
    /// Measured time in milliseconds.
    double ms = 0.0;
    double cut = 0.0; ///< optional budget/limit in ms (0 = none)

    /// Throughput of this entry in Mega-pixels/second.
    /// @return Mega-pixels processed per second.
    double mpix() const noexcept;
};

} // namespace bench
} // namespace iml

#endif