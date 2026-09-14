#pragma once
// imagelib/core/ExecutionPolicy.h
//
// Execution strategy selection. Algorithms in ImageLib follow a single
// hierarchy: Scalar -> SIMD -> Multithreaded -> SIMD + Multithreaded.
// The policy lets callers request a specific tier; Auto picks the fastest
// available given image size and hardware.

#include "imagelib/core/Types.h"

namespace iml {

enum class ExecutionMode : uint8 {
    Auto = 0,     // pick best path based on workload and hardware
    Serial,       // scalar, single threaded
    Simd,         // SIMD, single threaded
    Parallel,     // scalar, multithreaded
    SimdParallel, // SIMD + multithreaded
};

/// Describes how an algorithm should execute.
struct ExecutionPolicy {
    ExecutionMode mode      = ExecutionMode::Auto;
    uint32        maxWorkers = 0; // 0 => auto (hardware concurrency)
    uint32        grainSize  = 0; // 0 => auto (heuristic per algorithm)

    static constexpr ExecutionPolicy serial() noexcept { return {ExecutionMode::Serial, 1, 0}; }
    static constexpr ExecutionPolicy parallel() noexcept { return {ExecutionMode::Parallel, 0, 0}; }
    static constexpr ExecutionPolicy simd() noexcept { return {ExecutionMode::Simd, 1, 0}; }
    static constexpr ExecutionPolicy simdParallel() noexcept { return {ExecutionMode::SimdParallel, 0, 0}; }
};

} // namespace iml