// imagelib/threading/ParallelFor.tpp
//
// Template member definitions for ParallelFor.h. This file is included at
// the bottom of ParallelFor.h (never include it directly).

#ifndef IMAGELIB_THREADING_PARALLELFOR_H_
#error "Include ParallelFor.h, not ParallelFor.tpp directly."
#endif

#include <algorithm>
#include <cstdint>
#include <utility>

namespace iml {

template <class Fn>
void parallelFor(size_t begin, size_t end, Fn fn, const ExecutionPolicy& policy) {
    if (begin >= end) return;
    const size_t count = end - begin;
    if (!detail::wantsParallel(policy, count, parallelThreshold)) {
        for (size_t i = begin; i < end; ++i) fn(i);
        return;
    }
    ThreadPool& pool = detail::resolvePool(policy);
    if (policy.grainSize != 0) {
        detail::runGrains(pool, begin, end, policy.grainSize, fn);
    } else {
        const size_t workers = std::max<size_t>(1, pool.workerCount());
        const size_t grain   = std::max<size_t>(1, (count + workers - 1) / workers);
        detail::runGrains(pool, begin, end, grain, fn);
    }
}

template <class Fn>
void parallelFor(size_t n, Fn fn, const ExecutionPolicy& policy) {
    parallelFor(size_t(0), n, std::move(fn), policy);
}

template <class Fn>
void parallelForRows(size_t height, Fn fn, const ExecutionPolicy& policy) {
    if (height == 0) return;
    // Rows are the natural grain: one contiguous chunk of rows per worker.
    if (!detail::wantsParallel(policy, height, parallelThreshold)) {
        for (size_t r = 0; r < height; ++r) fn(r);
        return;
    }
    ThreadPool& pool   = detail::resolvePool(policy);
    const size_t workers = std::max<size_t>(1, pool.workerCount());
    const size_t grain   = std::max<size_t>(1, (height + workers - 1) / workers);
    detail::runGrains(pool, 0, height, grain, fn);
}

template <class Fn>
void parallelForColumns(size_t width, Fn fn, const ExecutionPolicy& policy) {
    if (width == 0) return;
    if (!detail::wantsParallel(policy, width, parallelThreshold)) {
        for (size_t c = 0; c < width; ++c) fn(c);
        return;
    }
    ThreadPool& pool   = detail::resolvePool(policy);
    const size_t workers = std::max<size_t>(1, pool.workerCount());
    const size_t grain   = std::max<size_t>(1, (width + workers - 1) / workers);
    detail::runGrains(pool, 0, width, grain, fn);
}

namespace detail {

template <class Fn>
void runGrains(ThreadPool& pool, size_t base, size_t end, size_t grain, Fn& fn) {
    const size_t n = end - base;
    if (n == 0) return;
    const size_t g      = grain > 0 ? grain : std::max<size_t>(1, end - base);
    const size_t grains = (n + g - 1) / g;
    if (grains <= 1) {
        for (size_t i = base; i < end; ++i) fn(i);
        return;
    }
    size_t start    = base;
    size_t remaining = n;
    for (size_t gr = 0; gr < grains - 1; ++gr) {
        const size_t gStart = start;
        start    += g;
        remaining -= g;
        pool.push(Job([&fn, gStart, g]() {
            for (size_t k = 0; k < g; ++k) fn(gStart + k);
        }));
    }
    const size_t lastStart = start;
    const size_t lastCount = remaining;
    pool.push(Job([&fn, lastStart, lastCount]() {
        for (size_t k = 0; k < lastCount; ++k) fn(lastStart + k);
    }));
    pool.waitAll();
}

} // namespace detail

} // namespace iml