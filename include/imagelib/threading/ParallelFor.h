#pragma once
#define IMAGELIB_THREADING_PARALLELFOR_H_
/// @file ParallelFor.h
/// Parallel range loops over the default thread pool, honoring
/// ExecutionPolicy. SIMD-aware variants split ranges into vector blocks.

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
/// @tparam Fn Callable invoked as fn(index).
/// @param begin First index.
/// @param end One past the last index.
/// @param fn Callback.
/// @param policy Execution policy.
template <class Fn>
void parallelFor(
    size_t begin,
    size_t end,
    Fn fn,
    const ExecutionPolicy& policy = ExecutionPolicy());

/// Convenience overload wrapping a 0-based range.
/// @tparam Fn Callable invoked as fn(index).
/// @param n Number of indices [0, n).
/// @param fn Callback.
/// @param policy Execution policy.
template <class Fn>
void parallelFor(size_t n, Fn fn, const ExecutionPolicy& policy = ExecutionPolicy());

/// Loop over row indices [0, height), calling fn(row).
/// @tparam Fn Callable invoked as fn(row).
/// @param height Number of rows.
/// @param fn Callback.
/// @param policy Execution policy.
template <class Fn>
void parallelForRows(size_t height, Fn fn, const ExecutionPolicy& policy = ExecutionPolicy());

/// Loop over column indices [0, width), calling fn(col).
/// @tparam Fn Callable invoked as fn(col).
/// @param width Number of columns.
/// @param fn Callback.
/// @param policy Execution policy.
template <class Fn>
void parallelForColumns(size_t width, Fn fn, const ExecutionPolicy& policy = ExecutionPolicy());

/// Each parallelFor* has a SIMD twin that runs `simdFn` once per
/// Vec::Lanes-wide block and `scalarFn` once per leftover tail element.
/// Vec is any simd::SimdVec<T, N>.

/// SIMD-aware variant of parallelFor over [begin, end).
/// @tparam Vec simd::SimdVec type whose Lanes gives the block width.
/// @tparam SimdFn Callable invoked as simdFn(blockStart) per vector block.
/// @tparam ScalarFn Callable invoked as scalarFn(index) per tail element.
/// @param begin First index.
/// @param end One past the last index.
/// @param simdFn Per-vector-block callback.
/// @param scalarFn Per-tail-element callback.
/// @param policy Execution policy.
template <class Vec, class SimdFn, class ScalarFn>
void parallelForSimd(
    size_t begin,
    size_t end,
    SimdFn simdFn,
    ScalarFn scalarFn,
    const ExecutionPolicy& policy = ExecutionPolicy());

/// Convenience overload wrapping a 0-based range.
/// @tparam Vec simd::SimdVec type whose Lanes gives the block width.
/// @tparam SimdFn Callable invoked as simdFn(blockStart) per vector block.
/// @tparam ScalarFn Callable invoked as scalarFn(index) per tail element.
/// @param n Number of indices [0, n).
/// @param simdFn Per-vector-block callback.
/// @param scalarFn Per-tail-element callback.
/// @param policy Execution policy.
template <class Vec, class SimdFn, class ScalarFn>
void parallelForSimd(
    size_t n,
    SimdFn simdFn,
    ScalarFn scalarFn,
    const ExecutionPolicy& policy = ExecutionPolicy());

/// SIMD-aware variant of parallelForRows: splits every row into
/// Vec::Lanes-wide column blocks.
/// @tparam Vec simd::SimdVec type whose Lanes gives the block width.
/// @tparam SimdFn Callable invoked as simdFn(row, colStart).
/// @tparam ScalarFn Callable invoked as scalarFn(row, col).
/// @param width Row length.
/// @param height Number of rows.
/// @param simdFn Per-vector-block callback.
/// @param scalarFn Per-tail-element callback.
/// @param policy Execution policy.
template <class Vec, class SimdFn, class ScalarFn>
void parallelForRowsSimd(
    size_t width,
    size_t height,
    SimdFn simdFn,
    ScalarFn scalarFn,
    const ExecutionPolicy& policy = ExecutionPolicy());

/// SIMD-aware variant of parallelForColumns: splits every column into
/// Vec::Lanes-wide row blocks.
/// @tparam Vec simd::SimdVec type whose Lanes gives the block width.
/// @tparam SimdFn Callable invoked as simdFn(col, rowStart).
/// @tparam ScalarFn Callable invoked as scalarFn(col, row).
/// @param height Column length.
/// @param width Number of columns.
/// @param simdFn Per-vector-block callback.
/// @param scalarFn Per-tail-element callback.
/// @param policy Execution policy.
template <class Vec, class SimdFn, class ScalarFn>
void parallelForColumnsSimd(
    size_t height,
    size_t width,
    SimdFn simdFn,
    ScalarFn scalarFn,
    const ExecutionPolicy& policy = ExecutionPolicy());

namespace detail {

/// True when the policy wants pooled execution for `count` elements.
bool wantsParallel(const ExecutionPolicy& policy, size_t count, size_t threshold) noexcept;

/// Split [base, end) into contiguous grains of work and run them over the pool.
template <class Fn> void runGrains(ThreadPool& pool, size_t base, size_t end, size_t grain, Fn& fn);

/// Resolve which pool a policy wants to run on; caller owns the returned pool
/// when it differs from the default.
ThreadPool& resolvePool(const ExecutionPolicy& policy);

} // namespace detail

} // namespace iml

#include "threading/ParallelFor.tpp"