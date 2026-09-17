#ifndef IMAGELIB_BENCHMARK_MEASURE_H_
#error "Include Measure.h, not Measure.tpp directly."
#endif

#include <utility>

namespace iml {
namespace bench {

/// Times a single invocation of `fn`.
/// @tparam Fn Invocable type.
/// @param fn Callable to time.
/// @return Elapsed time in milliseconds.
template <class Fn> inline double timeOnce(Fn&& fn) {
    const double t0 = nowMs();
    fn();
    return nowMs() - t0;
}

/// Runs `fn` `warmup` times, then `iterations` measured runs, returning the
/// best (minimum) time in milliseconds; warm-up lets caches, branch predictors
/// and the thread pool settle before timing.
/// @tparam Fn Invocable type.
/// @param iterations Number of measured runs.
/// @param warmup Number of warm-up runs.
/// @param fn Callable to run.
/// @return Best (minimum) elapsed time in milliseconds.
template <class Fn> inline double bestMs(int iterations, int warmup, Fn&& fn) {
    if (iterations < 1)
        iterations = 1;
    for (int i = 0; i < warmup; ++i)
        fn();
    double best = timeOnce(fn);
    for (int i = 1; i < iterations; ++i) {
        const double t = timeOnce(fn);
        if (t < best)
            best = t;
    }
    return best;
}

/// Runs `fn` `iterations` times with `warmup` warm-ups, returning the best time.
/// @tparam Fn Invocable type.
/// @param fn Callable to run.
/// @param iterations Number of measured runs.
/// @param warmup Number of warm-up runs.
/// @return Best (minimum) elapsed time in milliseconds.
template <class Fn> inline double bestMs(Fn&& fn, int iterations, int warmup) {
    return bestMs(iterations, warmup, std::forward<Fn>(fn));
}

} // namespace bench
} // namespace iml