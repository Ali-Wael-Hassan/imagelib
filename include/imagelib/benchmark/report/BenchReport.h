#pragma once
#ifndef IMAGELIB_BENCHMARK_BENCHREPORT_H_
#define IMAGELIB_BENCHMARK_BENCHREPORT_H_

/// @file imagelib/benchmark/report/BenchReport.h
/// Accumulates named measurements and prints aligned reports.

#include "imagelib/benchmark/record/Entry.h"
#include "imagelib/benchmark/record/PairEntry.h"

#include <string>
#include <vector>

namespace iml {
namespace bench {

/// Accumulates named measurements and prints an aligned report.
class BenchReport {
  public:
    /// Measures `fn` (best of `iterations`, with `warmup` warm-ups) and
    /// appends it. `pixels` is used for the throughput column; pass the image
    /// size even when the kernel's unit of work differs.
    /// @tparam Fn Invocable type.
    /// @param name Report row label.
    /// @param pixels Pixel count driving the throughput column.
    /// @param iterations Number of measured runs.
    /// @param warmup Number of warm-up runs.
    /// @param fn Callable to benchmark.
    template <class Fn>
    void run(const char* name, uint64 pixels, int iterations, int warmup, Fn&& fn);

    /// Same as run() plus a budget: rows whose time exceeds `budgetMs` are
    /// flagged with a warning trailer.
    /// @tparam Fn Invocable type.
    /// @param name Report row label.
    /// @param pixels Pixel count driving the throughput column.
    /// @param iterations Number of measured runs.
    /// @param budgetMs Budget limit in milliseconds (0 = none).
    /// @param fn Callable to benchmark.
    /// @param warmup Number of warm-up runs (default 1).
    template <class Fn>
    void runBudget(
        const char* name,
        uint64 pixels,
        int iterations,
        double budgetMs,
        Fn&& fn,
        int warmup = 1);

    /// Adds a pre-measured entry (handy when you already timed the kernel).
    /// @param name Report row label.
    /// @param pixels Pixel count driving the throughput column.
    /// @param ms Measured time in milliseconds.
    /// @param budgetMs Budget limit in milliseconds (0 = none).
    void add(const char* name, uint64 pixels, double ms, double budgetMs = 0.0);

    /// Measures the SAME kernel under two policies and stores them as one row
    /// with serial/parallel columns (see printPairs). `serialFn` runs first
    /// and becomes the speedup baseline.
    /// @tparam FnS Serial-policy callable type.
    /// @tparam FnP Parallel-policy callable type.
    /// @param name Report row label.
    /// @param pixels Pixel count driving the throughput columns.
    /// @param iterations Number of measured runs.
    /// @param warmup Number of warm-up runs.
    /// @param serialFn Serial-policy callable (speedup baseline).
    /// @param parallelFn Parallel-policy callable.
    template <class FnS, class FnP>
    void pair(
        const char* name,
        uint64 pixels,
        int iterations,
        int warmup,
        FnS&& serialFn,
        FnP&& parallelFn);

    /// Same as pair() with a budget on the parallel time (ms); rows missing
    /// the budget are flagged with a warning trailer.
    /// @tparam FnS Serial-policy callable type.
    /// @tparam FnP Parallel-policy callable type.
    /// @param name Report row label.
    /// @param pixels Pixel count driving the throughput columns.
    /// @param iterations Number of measured runs.
    /// @param budgetMs Budget limit in milliseconds (0 = none).
    /// @param serialFn Serial-policy callable (speedup baseline).
    /// @param parallelFn Parallel-policy callable.
    /// @param warmup Number of warm-up runs (default 1).
    template <class FnS, class FnP>
    void pairBudget(
        const char* name,
        uint64 pixels,
        int iterations,
        double budgetMs,
        FnS&& serialFn,
        FnP&& parallelFn,
        int warmup = 1);

    /// Returns whether no measurements have been recorded.
    /// @return True when both entry and pair lists are empty.
    bool empty() const noexcept;

    /// Number of single-entry rows.
    /// @return Number of recorded entries.
    size_t size() const noexcept;
    /// Accesses the entry at index `i`.
    /// @param i Zero-based entry index.
    /// @return Const reference to the entry at `i`.
    const Entry& operator[](size_t i) const noexcept;

    /// Prints the report. The first entry becomes the baseline for the
    /// "speedup vs first" column. The name column widens to fit the longest
    /// label so the columns stay aligned for any kernel naming.
    void print() const;

    /// Prints only a single-line summary (used when only one measurement).
    /// @param i Zero-based entry index (default 0).
    void printEntry(size_t i = 0) const;

    /// Number of serial/parallel pair rows.
    /// @return Number of recorded pairs.
    size_t pairSize() const noexcept;
    /// Accesses the pair at index `i`.
    /// @param i Zero-based pair index.
    /// @return Const reference to the pair at `i`.
    const PairEntry& pairAt(size_t i) const noexcept;
    /// Label of the pair at index `i`.
    /// @param i Zero-based pair index.
    /// @return Const reference to the pair's name.
    const std::string& pairLabel(size_t i) const noexcept;

    /// Prints the serial-vs-parallel table: one row per kernel with separate
    /// columns for ms and Mpix/s. The "speedup" column is serial/parallel and
    /// the name column widens to fit the longest label.
    void printPairs() const;

  private:
    /// Single-entry measurement rows.
    std::vector<Entry> entries_;
    /// Serial/parallel measurement pair rows.
    std::vector<PairEntry> pairs_;
};

} // namespace bench
} // namespace iml

#include "benchmark/report/BenchReport.tpp"
#endif