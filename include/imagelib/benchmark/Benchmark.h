#pragma once
// imagelib/benchmark/Benchmark.h
//
// Small, reusable benchmarking utility used by imagelib_bench and every
// example/demo. It measures wall-clock time with warm-up + best-of-N runs and
// formats results as ms and Mega-pixels/second, so students can read their
// real kernel performance without hand-rolling timers.
//
// Typical use:
//
//   using namespace iml::bench;
//   auto ms = bestMs(5, [&] { proc::filter::invert(src.view(), dst.view()); });
//   printf("invert: best %.2f ms (%.1f Mpix/s)\n", ms, mpixPerSec(src.w*src.h, ms));
//
//   // richer, self-formatting comparison across execution policies:
//   BenchReport report;
//   report.run("invert (serial)",     pixels, 5, [&]{ ... serial ... });
//   report.run("invert (simd+par)",   pixels, 5, [&]{ ... simdParallel ... });
//   report.print();  // aligned table, ms + Mpix/s + speedup vs. first row
//
// Layout: template bodies live in Benchmark.tpp (included below), all
// non-template definitions live in src/benchmark/Benchmark.cpp.

#define IMAGELIB_BENCHMARK_BENCHMARK_H_

#include "imagelib/core/Types.h"
#include "imagelib/core/Image.h"

#include <chrono>
#include <string>
#include <vector>

namespace iml {
namespace bench {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

/// Reads a steady clock in milliseconds (costs ~20 ns; negligible vs kernels).
double nowMs() noexcept;

/// Throughput of `pixels` processed in `ms`, in Mega-pixels/second.
double mpixPerSec(uint64 pixels, double ms) noexcept;

/// One measured entry.
struct Entry {
    std::string name;
    uint64      pixels = 0;
    double      ms = 0.0;
    double      cut = 0.0; ///< optional budget/limit in ms (0 = none)

    double mpix() const noexcept;
};

/// A serial-vs-parallel measurement of the same kernel (one table row).
struct PairEntry {
    std::string name;
    uint64      pixels = 0;    ///< pixel count used for both throughputs
    double      msSerial = 0.0;
    double      msParallel = 0.0;
    double      cut = 0.0;     ///< optional budget/limit in ms (0 = none)

    double mpixSerial()   const noexcept;
    double mpixParallel() const noexcept;
    /// Parallel speedup over the serial baseline (1.0 = no gain).
    double speedup() const noexcept;
};

/// Accumulates named measurements and prints an aligned report.
class BenchReport {
public:
    // Template members are defined in Benchmark.tpp.

    /// Measures `fn` (best of `iterations`, with `warmup` warm-ups) and
    /// appends it. `pixels` is used for the throughput column; pass the image
    /// size even when the kernel's unit of work differs.
    template <class Fn> void run(const char* name, uint64 pixels,
                                 int iterations, int warmup, Fn&& fn);

    /// Same as run() plus a budget: rows whose time exceeds `budgetMs` are
    /// flagged with a warning trailer.
    template <class Fn> void runBudget(const char* name, uint64 pixels,
                                       int iterations, double budgetMs,
                                       Fn&& fn, int warmup = 1);

    /// Adds a pre-measured entry (handy when you already timed the kernel).
    void add(const char* name, uint64 pixels, double ms, double budgetMs = 0.0);

    /// Measures the SAME kernel under two policies and stores them as one row
    /// with serial/parallel columns (see printPairs). `serialFn` runs first
    /// and becomes the speedup baseline.
    template <class FnS, class FnP>
    void pair(const char* name, uint64 pixels, int iterations, int warmup,
              FnS&& serialFn, FnP&& parallelFn);

    /// Same as pair() with a budget on the parallel time (ms); rows missing
    /// the budget are flagged with a warning trailer.
    template <class FnS, class FnP>
    void pairBudget(const char* name, uint64 pixels, int iterations,
                    double budgetMs, FnS&& serialFn, FnP&& parallelFn,
                    int warmup = 1);

    bool empty() const noexcept;

    // -- single-entry rows -------------------------------------------------

    size_t size() const noexcept;
    const Entry& operator[](size_t i) const noexcept;

    /// Prints the report. The first entry becomes the baseline for the
    /// "speedup vs first" column. The name column widens to fit the longest
    /// label so the columns stay aligned for any kernel naming.
    void print() const;

    /// Prints only a single-line summary (used when only one measurement).
    void printEntry(size_t i = 0) const;

    // -- serial/parallel pair rows -----------------------------------------

    size_t pairSize() const noexcept;
    const PairEntry& pairAt(size_t i) const noexcept;
    const std::string& pairLabel(size_t i) const noexcept;

    /// Prints the serial-vs-parallel table: one row per kernel with separate
    /// columns for ms and Mpix/s. The "speedup" column is serial/parallel and
    /// the name column widens to fit the longest label.
    void printPairs() const;

private:
    std::vector<Entry>     entries_;
    std::vector<PairEntry> pairs_;
};

/// Fills `dst` with a deterministic synthetic RGB gradient (handy for stable
/// benchmarks that never touch disk).
void fillGradient(Image& dst);

} // namespace bench
} // namespace iml

#include "imagelib/benchmark/Benchmark.tpp"