#pragma once
#define IMAGELIB_THREADING_PARALLELFOR_H_
// imagelib/threading/ParallelFor.h
//
// Parallel range loops over the default thread pool, honoring ExecutionPolicy:
//   Serial      -> inline loop
//   Parallel    -> worker pool, split into grains
//   Simd/...    -> parallelized like Parallel (SIMD selection is orthogonal
//                  and happens inside the kernel, not the executor)
//   Auto        -> pool when the range is large enough to amortize dispatch
//
// When policy.maxWorkers > 0 and it differs from the default pool size, a
// temporary pool of exactly that size is spun up for the call.

#include "imagelib/core/ExecutionPolicy.h"
#include "imagelib/threading/ThreadPool.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace iml {

/// Elements per range before Auto switches to the pool.
constexpr size_t parallelThreshold = 1u << 16;

/// Loop over [begin, end), calling fn(i) with indices in an unspecified order.
/// fn is called exactly once per index regardless of the execution path.
template <class Fn>
void parallelFor(size_t begin, size_t end, Fn fn, const ExecutionPolicy& policy = ExecutionPolicy());

/// Convenience overload wrapping a 0-based range.
template <class Fn>
void parallelFor(size_t n, Fn fn, const ExecutionPolicy& policy = ExecutionPolicy());

/// Loop over row indices [0, height), calling fn(row). Rows of an Image are
/// independent by construction (constant row stride), so this maps straight
/// onto the pool.
template <class Fn>
void parallelForRows(size_t height, Fn fn, const ExecutionPolicy& policy = ExecutionPolicy());

/// Loop over column indices [0, width), calling fn(col). Columns touch strided
/// memory (one sample per row), but writes to distinct columns are independent,
/// so this maps straight onto the pool.
template <class Fn>
void parallelForColumns(size_t width, Fn fn, const ExecutionPolicy& policy = ExecutionPolicy());

namespace detail {

bool wantsParallel(const ExecutionPolicy& policy, size_t count, size_t threshold) noexcept;

/// Split [base, end) into contiguous grains of work and run them over the pool.
template <class Fn>
void runGrains(ThreadPool& pool, size_t base, size_t end, size_t grain, Fn& fn);

/// Resolve which pool a policy wants to run on; caller owns the returned pool
/// when it differs from the default.
ThreadPool& resolvePool(const ExecutionPolicy& policy);

} // namespace detail

} // namespace iml

#include "imagelib/threading/ParallelFor.tpp"