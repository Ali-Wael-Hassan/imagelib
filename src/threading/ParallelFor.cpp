// imagelib/src/threading/ParallelFor.cpp
//
// Out-of-line non-template helpers used by ParallelFor.h: the policy -> pool
// decision (wantsParallel) and the pool resolution that honors maxWorkers.

#include "imagelib/threading/ParallelFor.h"

#include <algorithm>
#include <cstdint>

namespace iml {
namespace detail {

bool wantsParallel(const ExecutionPolicy& policy, size_t count, size_t threshold) noexcept {
    switch (policy.mode) {
        case ExecutionMode::Serial:
        case ExecutionMode::Simd: // single-threaded SIMD: no thread pool
            return false;
        case ExecutionMode::Parallel:
        case ExecutionMode::SimdParallel:
            return true;
        case ExecutionMode::Auto:
        default:
            return count >= threshold;
    }
}

namespace {

/// Process-wide pool used when a caller requests an explicit worker count
/// that differs from the default pool's. Spun up (or resized) on first use.
ThreadPool& temporaryPool(size_t workers) {
    static ThreadPool pool;
    if (pool.workerCount() != workers || !pool.running()) {
        pool.start(workers);
    }
    return pool;
}

} // namespace

ThreadPool& resolvePool(const ExecutionPolicy& policy) {
    ThreadPool& def = defaultThreadPool();
    if (policy.maxWorkers == 0 || policy.maxWorkers == def.workerCount()) {
        return def;
    }
    return temporaryPool(policy.maxWorkers);
}

} // namespace detail
} // namespace iml