#ifndef IMAGELIB_THREADING_PARALLELFOR_H_
#error "Include ParallelFor.h, not ParallelFor.tpp directly."
#endif

#include <algorithm>
#include <cstdint>
#include <utility>

namespace iml {

/// Loop over [begin, end), calling fn per index, inline or via the pool.
template <class Fn>
void parallelFor(size_t begin, size_t end, Fn fn, const ExecutionPolicy& policy) {
    if (begin >= end)
        return;
    const size_t count = end - begin;
    if (!detail::wantsParallel(policy, count, parallelThreshold)) {
        for (size_t i = begin; i < end; ++i)
            fn(i);
        return;
    }
    ThreadPool& pool = detail::resolvePool(policy);
    if (policy.grainSize != 0) {
        detail::runGrains(pool, begin, end, policy.grainSize, fn);
    } else {
        const size_t workers = std::max<size_t>(1, pool.workerCount());
        const size_t grain = std::max<size_t>(1, (count + workers - 1) / workers);
        detail::runGrains(pool, begin, end, grain, fn);
    }
}

/// 0-based convenience overload of parallelFor.
template <class Fn> void parallelFor(size_t n, Fn fn, const ExecutionPolicy& policy) {
    parallelFor(size_t(0), n, std::move(fn), policy);
}

/// Loop over row indices [0, height), calling fn(row).
template <class Fn> void parallelForRows(size_t height, Fn fn, const ExecutionPolicy& policy) {
    if (height == 0)
        return;
    if (!detail::wantsParallel(policy, height, parallelThreshold)) {
        for (size_t r = 0; r < height; ++r)
            fn(r);
        return;
    }
    ThreadPool& pool = detail::resolvePool(policy);
    const size_t workers = std::max<size_t>(1, pool.workerCount());
    const size_t grain = std::max<size_t>(1, (height + workers - 1) / workers);
    detail::runGrains(pool, 0, height, grain, fn);
}

/// Loop over column indices [0, width), calling fn(col).
template <class Fn> void parallelForColumns(size_t width, Fn fn, const ExecutionPolicy& policy) {
    if (width == 0)
        return;
    if (!detail::wantsParallel(policy, width, parallelThreshold)) {
        for (size_t c = 0; c < width; ++c)
            fn(c);
        return;
    }
    ThreadPool& pool = detail::resolvePool(policy);
    const size_t workers = std::max<size_t>(1, pool.workerCount());
    const size_t grain = std::max<size_t>(1, (width + workers - 1) / workers);
    detail::runGrains(pool, 0, width, grain, fn);
}

/// SIMD-aware loop over [begin, end) with per-vector-block and per-tail callbacks.
template <class Vec, class SimdFn, class ScalarFn>
void parallelForSimd(
    size_t begin,
    size_t end,
    SimdFn simdFn,
    ScalarFn scalarFn,
    const ExecutionPolicy& policy) {
    if (begin >= end)
        return;
    const size_t lanes = static_cast<size_t>(Vec::Lanes);
    const size_t count = end - begin;
    const size_t blocks = count / lanes;
    const size_t tailBeg = begin + blocks * lanes;

    if (!detail::wantsParallel(policy, count, parallelThreshold)) {
        for (size_t k = 0; k < blocks; ++k)
            simdFn(begin + k * lanes);
        for (size_t j = tailBeg; j < end; ++j)
            scalarFn(j);
        return;
    }
    ThreadPool& pool = detail::resolvePool(policy);
    const auto blockFn = [&](size_t k) { simdFn(begin + k * lanes); };
    if (policy.grainSize != 0) {
        detail::runGrains(pool, 0, blocks, policy.grainSize, blockFn);
    } else {
        const size_t workers = std::max<size_t>(1, pool.workerCount());
        const size_t grain = std::max<size_t>(1, (blocks + workers - 1) / workers);
        detail::runGrains(pool, 0, blocks, grain, blockFn);
    }
    for (size_t j = tailBeg; j < end; ++j)
        scalarFn(j);
}

/// 0-based convenience overload of parallelForSimd.
template <class Vec, class SimdFn, class ScalarFn>
void parallelForSimd(size_t n, SimdFn simdFn, ScalarFn scalarFn, const ExecutionPolicy& policy) {
    parallelForSimd<Vec>(size_t(0), n, std::move(simdFn), std::move(scalarFn), policy);
}

/// SIMD-aware variant of parallelForRows.
template <class Vec, class SimdFn, class ScalarFn>
void parallelForRowsSimd(
    size_t width,
    size_t height,
    SimdFn simdFn,
    ScalarFn scalarFn,
    const ExecutionPolicy& policy) {
    if (width == 0 || height == 0)
        return;
    const size_t lanes = static_cast<size_t>(Vec::Lanes);
    const size_t blocksPerRow = width / lanes;
    const size_t tailBegin = blocksPerRow * lanes;

    const auto rowFn = [&](size_t r) {
        for (size_t k = 0; k < blocksPerRow; ++k)
            simdFn(r, k * lanes);
        for (size_t x = tailBegin; x < width; ++x)
            scalarFn(r, x);
    };

    if (!detail::wantsParallel(policy, width * height, parallelThreshold)) {
        for (size_t r = 0; r < height; ++r)
            rowFn(r);
        return;
    }
    ThreadPool& pool = detail::resolvePool(policy);
    const size_t workers = std::max<size_t>(1, pool.workerCount());
    const size_t grain = policy.grainSize != 0
                             ? policy.grainSize
                             : std::max<size_t>(1, (height + workers - 1) / workers);
    detail::runGrains(pool, 0, height, grain, rowFn);
}

/// SIMD-aware variant of parallelForColumns.
template <class Vec, class SimdFn, class ScalarFn>
void parallelForColumnsSimd(
    size_t height,
    size_t width,
    SimdFn simdFn,
    ScalarFn scalarFn,
    const ExecutionPolicy& policy) {
    if (width == 0 || height == 0)
        return;
    const size_t lanes = static_cast<size_t>(Vec::Lanes);
    const size_t blocksPerCol = height / lanes;
    const size_t tailBegin = blocksPerCol * lanes;

    const auto colFn = [&](size_t c) {
        for (size_t k = 0; k < blocksPerCol; ++k)
            simdFn(c, k * lanes);
        for (size_t y = tailBegin; y < height; ++y)
            scalarFn(c, y);
    };

    if (!detail::wantsParallel(policy, height * width, parallelThreshold)) {
        for (size_t c = 0; c < width; ++c)
            colFn(c);
        return;
    }
    ThreadPool& pool = detail::resolvePool(policy);
    const size_t workers = std::max<size_t>(1, pool.workerCount());
    const size_t grain = policy.grainSize != 0
                             ? policy.grainSize
                             : std::max<size_t>(1, (width + workers - 1) / workers);
    detail::runGrains(pool, 0, width, grain, colFn);
}

namespace detail {

/// Splits [base, end) into contiguous grains run over the pool.
template <class Fn>
void runGrains(ThreadPool& pool, size_t base, size_t end, size_t grain, Fn& fn) {
    const size_t n = end - base;
    if (n == 0)
        return;
    const size_t g = grain > 0 ? grain : std::max<size_t>(1, end - base);
    const size_t grains = (n + g - 1) / g;
    if (grains <= 1) {
        for (size_t i = base; i < end; ++i)
            fn(i);
        return;
    }
    size_t start = base;
    size_t remaining = n;
    for (size_t gr = 0; gr < grains - 1; ++gr) {
        const size_t gStart = start;
        start += g;
        remaining -= g;
        pool.push(Job([&fn, gStart, g]() {
            for (size_t k = 0; k < g; ++k)
                fn(gStart + k);
        }));
    }
    const size_t lastStart = start;
    const size_t lastCount = remaining;
    pool.push(Job([&fn, lastStart, lastCount]() {
        for (size_t k = 0; k < lastCount; ++k)
            fn(lastStart + k);
    }));
    pool.waitAll();
}

} // namespace detail

} // namespace iml