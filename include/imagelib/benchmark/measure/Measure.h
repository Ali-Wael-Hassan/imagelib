#pragma once
#ifndef IMAGELIB_BENCHMARK_MEASURE_H_
#define IMAGELIB_BENCHMARK_MEASURE_H_

/// @file imagelib/benchmark/measure/Measure.h
/// Timing and throughput helpers: nowMs, mpixPerSec, timeOnce, bestMs.

#include "imagelib/core/Types.h"

#include <chrono>

namespace iml {
namespace bench {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

/// Reads the steady clock in milliseconds.
/// @return Current time in milliseconds.
double nowMs() noexcept;

/// Throughput of `pixels` processed in `ms`, in Mega-pixels/second.
/// @param pixels Number of pixels processed.
/// @param ms Elapsed time in milliseconds.
/// @return Mega-pixels processed per second.
double mpixPerSec(uint64 pixels, double ms) noexcept;

/// Times a single invocation of `fn`.
/// @tparam Fn Invocable type.
/// @param fn Callable to time.
/// @return Elapsed time in milliseconds.
template <class Fn> inline double timeOnce(Fn&& fn);

/// Runs `fn` `warmup` times, then `iterations` measured runs, returning the
/// best (minimum) time in milliseconds; warm-up lets caches, branch predictors
/// and the thread pool settle before timing.
/// @tparam Fn Invocable type.
/// @param iterations Number of measured runs.
/// @param warmup Number of warm-up runs.
/// @param fn Callable to run.
/// @return Best (minimum) elapsed time in milliseconds.
template <class Fn> inline double bestMs(int iterations, int warmup, Fn&& fn);

/// Runs `fn` `iterations` times with `warmup` warm-ups, returning the best time.
/// @tparam Fn Invocable type.
/// @param fn Callable to run.
/// @param iterations Number of measured runs (default 5).
/// @param warmup Number of warm-up runs (default 1).
/// @return Best (minimum) elapsed time in milliseconds.
template <class Fn> inline double bestMs(Fn&& fn, int iterations = 5, int warmup = 1);

} // namespace bench
} // namespace iml

#include "benchmark/measure/Measure.tpp"
#endif