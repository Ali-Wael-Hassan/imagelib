// imagelib/benchmark/Benchmark.tpp
//
// Template definitions for Benchmark.h. This file is included at the bottom
// of Benchmark.h (never include it directly).

#ifndef IMAGELIB_BENCHMARK_BENCHMARK_H_
#error "Include Benchmark.h, not Benchmark.tpp directly."
#endif

#include "imagelib/core/Types.h"

#include <utility>   // std::forward

namespace iml {
namespace bench {

// ---------------------------------------------------------------------------
// Free templates.
// ---------------------------------------------------------------------------

/// Time a single invocation of `fn` in milliseconds.
template <class Fn>
inline double timeOnce(Fn&& fn) {
    const double t0 = nowMs();
    fn();
    return nowMs() - t0;
}

/// Runs `fn` `warmup` times, then `iterations` measured runs, returning the
/// best (minimum) time in milliseconds. Warm-up lets caches, branch predictors
/// and the thread pool settle before timing.
template <class Fn>
inline double bestMs(int iterations, int warmup, Fn&& fn) {
    if (iterations < 1) iterations = 1;
    for (int i = 0; i < warmup; ++i) fn();
    double best = timeOnce(fn);
    for (int i = 1; i < iterations; ++i) {
        const double t = timeOnce(fn);
        if (t < best) best = t;
    }
    return best;
}

template <class Fn>
inline double bestMs(Fn&& fn, int iterations = 5, int warmup = 1) {
    return bestMs(iterations, warmup, std::forward<Fn>(fn));
}

// ---------------------------------------------------------------------------
// BenchReport template members.
// ---------------------------------------------------------------------------

template <class Fn>
void BenchReport::run(const char* name, uint64 pixels, int iterations,
                      int warmup, Fn&& fn) {
    const double ms = bestMs(iterations, warmup, std::forward<Fn>(fn));
    entries_.push_back({ name, pixels, ms, 0.0 });
}

template <class Fn>
void BenchReport::runBudget(const char* name, uint64 pixels, int iterations,
                            double budgetMs, Fn&& fn, int warmup) {
    const double ms = bestMs(iterations, warmup, std::forward<Fn>(fn));
    entries_.push_back({ name, pixels, ms, budgetMs });
}

template <class FnS, class FnP>
void BenchReport::pair(const char* name, uint64 pixels, int iterations,
                       int warmup, FnS&& serialFn, FnP&& parallelFn) {
    const double msS = bestMs(iterations, warmup, std::forward<FnS>(serialFn));
    const double msP = bestMs(iterations, warmup, std::forward<FnP>(parallelFn));
    pairs_.push_back({ name, pixels, msS, msP, 0.0 });
}

template <class FnS, class FnP>
void BenchReport::pairBudget(const char* name, uint64 pixels, int iterations,
                             double budgetMs, FnS&& serialFn, FnP&& parallelFn,
                             int warmup) {
    const double msS = bestMs(iterations, warmup, std::forward<FnS>(serialFn));
    const double msP = bestMs(iterations, warmup, std::forward<FnP>(parallelFn));
    pairs_.push_back({ name, pixels, msS, msP, budgetMs });
}

} // namespace bench
} // namespace iml