#include "imagelib/benchmark/report/BenchReport.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace iml {
namespace bench {

/// Adds a pre-measured entry (handy when you already timed the kernel).
/// @param name Report row label.
/// @param pixels Pixel count driving the throughput column.
/// @param ms Measured time in milliseconds.
/// @param budgetMs Budget limit in milliseconds (0 = none).
void BenchReport::add(const char* name, uint64 pixels, double ms, double budgetMs) {
    entries_.push_back({name, pixels, ms, budgetMs});
}

/// Returns whether no measurements have been recorded.
/// @return True when both entry and pair lists are empty.
bool BenchReport::empty() const noexcept { return entries_.empty() && pairs_.empty(); }

/// Number of single-entry rows.
/// @return Number of recorded entries.
size_t BenchReport::size() const noexcept { return entries_.size(); }

/// Accesses the entry at index `i`.
/// @param i Zero-based entry index.
/// @return Const reference to the entry at `i`.
const Entry& BenchReport::operator[](size_t i) const noexcept { return entries_[i]; }

/// Number of serial/parallel pair rows.
/// @return Number of recorded pairs.
size_t BenchReport::pairSize() const noexcept { return pairs_.size(); }

/// Accesses the pair at index `i`.
/// @param i Zero-based pair index.
/// @return Const reference to the pair at `i`.
const PairEntry& BenchReport::pairAt(size_t i) const noexcept { return pairs_[i]; }

/// Label of the pair at index `i`.
/// @param i Zero-based pair index.
/// @return Const reference to the pair's name.
const std::string& BenchReport::pairLabel(size_t i) const noexcept { return pairs_[i].name; }

/// Prints the single-entry report: throughput and "vs first" speedup columns.
void BenchReport::print() const {
    if (entries_.empty())
        return;
    const double base = entries_[0].ms > 0.0 ? entries_[0].ms : 1e-9;

    size_t nameW = std::string("benchmark").size();
    for (const Entry& e : entries_)
        nameW = std::max(nameW, e.name.size());

    std::printf(
        "%-*s %10s %12s %14s\n",
        static_cast<int>(nameW),
        "benchmark",
        "ms (best)",
        "Mpix/s",
        "vs first");
    const int rule = static_cast<int>(nameW) + 1 + 10 + 1 + 12 + 1 + 14;
    for (int i = 0; i < rule; ++i)
        std::printf("-");
    std::printf("\n");
    for (const Entry& e : entries_) {
        const double speedup = base / e.ms;
        std::printf(
            "%-*s %10.4f %12.1f %13.2fx%s\n",
            static_cast<int>(nameW),
            e.name.c_str(),
            e.ms,
            e.mpix(),
            speedup,
            (e.cut > 0.0 && e.ms > e.cut) ? "   <-- over budget" : "");
    }
}

/// Prints only a single-line summary (used when only one measurement).
/// @param i Zero-based entry index (default 0).
void BenchReport::printEntry(size_t i) const {
    if (i >= entries_.size())
        return;
    const Entry& e = entries_[i];
    std::printf(
        "%-*s %10.4f ms   %8.1f Mpix/s\n",
        static_cast<int>(std::max<size_t>(9, e.name.size())),
        e.name.c_str(),
        e.ms,
        e.mpix());
}

/// Prints the serial-vs-parallel table: one row per kernel with separate
/// columns for ms and Mpix/s, plus a speedup column.
void BenchReport::printPairs() const {
    if (pairs_.empty())
        return;

    size_t nameW = std::string("benchmark").size();
    for (const PairEntry& e : pairs_)
        nameW = std::max(nameW, e.name.size());

    std::printf(
        "%-*s %12s %12s %12s %12s %10s\n",
        static_cast<int>(nameW),
        "benchmark",
        "serial ms",
        "serial Mpix/s",
        "parallel ms",
        "parallel Mpix/s",
        "speedup");
    const int rule = static_cast<int>(nameW) + 1 + 12 + 1 + 12 + 1 + 12 + 1 + 12 + 1 + 10;
    for (int i = 0; i < rule; ++i)
        std::printf("-");
    std::printf("\n");
    for (const PairEntry& e : pairs_) {
        std::printf(
            "%-*s %12.4f %12.1f %12.4f %12.1f %9.2fx%s\n",
            static_cast<int>(nameW),
            e.name.c_str(),
            e.msSerial,
            e.mpixSerial(),
            e.msParallel,
            e.mpixParallel(),
            e.speedup(),
            (e.cut > 0.0 && e.msParallel > e.cut) ? "   <-- over budget" : "");
    }
}

} // namespace bench
} // namespace iml