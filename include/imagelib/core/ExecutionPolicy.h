#pragma once
/// @file
/// Execution strategy selection for algorithms in ImageLib.

#include "imagelib/core/Types.h"

namespace iml {

/// Execution tier requested by a caller.
enum class ExecutionMode : uint8 {
    /// Pick the best path based on workload and hardware.
    Auto = 0,
    /// Scalar, single threaded.
    Serial,
    /// SIMD, single threaded.
    Simd,
    /// Scalar, multithreaded.
    Parallel,
    /// SIMD + multithreaded.
    SimdParallel,
};

/// Describes how an algorithm should execute.
struct ExecutionPolicy {
    /// Requested execution mode.
    ExecutionMode mode = ExecutionMode::Auto;
    /// Maximum worker threads; 0 = auto (hardware concurrency).
    uint32 maxWorkers = 0;
    /// Work grain size; 0 = auto (heuristic per algorithm).
    uint32 grainSize = 0;

    /// Returns a single-threaded scalar policy.
    /// @return The configured ExecutionPolicy.
    static constexpr ExecutionPolicy serial() noexcept { return {ExecutionMode::Serial, 1, 0}; }
    /// Returns a multithreaded scalar policy.
    /// @return The configured ExecutionPolicy.
    static constexpr ExecutionPolicy parallel() noexcept { return {ExecutionMode::Parallel, 0, 0}; }
    /// Returns a single-threaded SIMD policy.
    /// @return The configured ExecutionPolicy.
    static constexpr ExecutionPolicy simd() noexcept { return {ExecutionMode::Simd, 1, 0}; }
    /// Returns a SIMD + multithreaded policy.
    /// @return The configured ExecutionPolicy.
    static constexpr ExecutionPolicy simdParallel() noexcept {
        return {ExecutionMode::SimdParallel, 0, 0};
    }
};

} // namespace iml