#ifndef IMAGELIB_BENCHMARK_BENCHREPORT_H_
#error "Include BenchReport.h, not BenchReport.tpp directly."
#endif

#include "imagelib/benchmark/measure/Measure.h"

#include <utility>

namespace iml {
namespace bench {

/// Measures `fn` best-of-`iterations` and appends a single-entry row.
/// @tparam Fn Invocable type.
/// @param name Report row label.
/// @param pixels Pixel count driving the throughput column.
/// @param iterations Number of measured runs.
/// @param warmup Number of warm-up runs.
/// @param fn Callable to benchmark.
template <class Fn>
void BenchReport::run(const char* name, uint64 pixels, int iterations, int warmup, Fn&& fn) {
    const double ms = bestMs(iterations, warmup, std::forward<Fn>(fn));
    entries_.push_back({name, pixels, ms, 0.0});
}

/// Measures `fn` best-of-`iterations` and appends a single-entry row with a
/// budget; rows exceeding `budgetMs` are flagged with a warning trailer.
/// @tparam Fn Invocable type.
/// @param name Report row label.
/// @param pixels Pixel count driving the throughput column.
/// @param iterations Number of measured runs.
/// @param budgetMs Budget limit in milliseconds (0 = none).
/// @param fn Callable to benchmark.
/// @param warmup Number of warm-up runs (default 1).
template <class Fn>
void BenchReport::runBudget(
    const char* name,
    uint64 pixels,
    int iterations,
    double budgetMs,
    Fn&& fn,
    int warmup) {
    const double ms = bestMs(iterations, warmup, std::forward<Fn>(fn));
    entries_.push_back({name, pixels, ms, budgetMs});
}

/// Measures the same kernel under serial and parallel policies and appends
/// one pair row; `serialFn` runs first and becomes the speedup baseline.
/// @tparam FnS Serial-policy callable type.
/// @tparam FnP Parallel-policy callable type.
/// @param name Report row label.
/// @param pixels Pixel count driving the throughput columns.
/// @param iterations Number of measured runs.
/// @param warmup Number of warm-up runs.
/// @param serialFn Serial-policy callable (speedup baseline).
/// @param parallelFn Parallel-policy callable.
template <class FnS, class FnP>
void BenchReport::pair(
    const char* name,
    uint64 pixels,
    int iterations,
    int warmup,
    FnS&& serialFn,
    FnP&& parallelFn) {
    const double msS = bestMs(iterations, warmup, std::forward<FnS>(serialFn));
    const double msP = bestMs(iterations, warmup, std::forward<FnP>(parallelFn));
    pairs_.push_back({name, pixels, msS, msP, 0.0});
}

/// Measures the same kernel under both policies with a budget on the parallel
/// time; rows exceeding `budgetMs` are flagged with a warning trailer.
/// @tparam FnS Serial-policy callable type.
/// @tparam FnP Parallel-policy callable type.
/// @param name Report row label.
/// @param pixels Pixel count driving the throughput columns.
/// @param iterations Number of measured runs.
/// @param budgetMs Budget limit in milliseconds (0 = none).
/// @param serialFn Serial-policy callable (speedup baseline).
/// @param parallelFn Parallel-policy callable.
/// @param warmup Number of warm-up runs (default 1).
template <class FnS, class FnP>
void BenchReport::pairBudget(
    const char* name,
    uint64 pixels,
    int iterations,
    double budgetMs,
    FnS&& serialFn,
    FnP&& parallelFn,
    int warmup) {
    const double msS = bestMs(iterations, warmup, std::forward<FnS>(serialFn));
    const double msP = bestMs(iterations, warmup, std::forward<FnP>(parallelFn));
    pairs_.push_back({name, pixels, msS, msP, budgetMs});
}

} // namespace bench
} // namespace iml