// src/benchmark/Benchmark.cpp
//
// Non-template definitions for the iml::bench utility (declared in
// include/imagelib/benchmark/Benchmark.h; template bodies are in the .tpp).
#include "imagelib/benchmark/Benchmark.h"

#include <algorithm>   // std::max
#include <cstdio>      // std::printf
#include <string>

namespace iml {
namespace bench {

double nowMs() noexcept {
    return std::chrono::duration<double, std::milli>(
        Clock::now().time_since_epoch()).count();
}

double mpixPerSec(uint64 pixels, double ms) noexcept {
    return static_cast<double>(pixels) / (ms * 1000.0);
}

double Entry::mpix() const noexcept {
    return mpixPerSec(pixels, ms);
}

double PairEntry::mpixSerial() const noexcept {
    return mpixPerSec(pixels, msSerial);
}

double PairEntry::mpixParallel() const noexcept {
    return mpixPerSec(pixels, msParallel);
}

double PairEntry::speedup() const noexcept {
    return msSerial > 0.0 ? msSerial / (msParallel > 0.0 ? msParallel : 1e-9) : 1.0;
}

void BenchReport::add(const char* name, uint64 pixels, double ms, double budgetMs) {
    entries_.push_back({ name, pixels, ms, budgetMs });
}

bool BenchReport::empty() const noexcept {
    return entries_.empty() && pairs_.empty();
}

size_t BenchReport::size() const noexcept {
    return entries_.size();
}

const Entry& BenchReport::operator[](size_t i) const noexcept {
    return entries_[i];
}

size_t BenchReport::pairSize() const noexcept {
    return pairs_.size();
}

const PairEntry& BenchReport::pairAt(size_t i) const noexcept {
    return pairs_[i];
}

const std::string& BenchReport::pairLabel(size_t i) const noexcept {
    return pairs_[i].name;
}

void BenchReport::print() const {
    if (entries_.empty()) return;
    const double base = entries_[0].ms > 0.0 ? entries_[0].ms : 1e-9;

    // Longest label (header vs any entry) sets the name column width.
    size_t nameW = std::string("benchmark").size();
    for (const Entry& e : entries_) nameW = std::max(nameW, e.name.size());

    std::printf("%-*s %10s %12s %14s\n", static_cast<int>(nameW),
                "benchmark", "ms (best)", "Mpix/s", "vs first");
    const int rule = static_cast<int>(nameW) + 1 + 10 + 1 + 12 + 1 + 14;
    for (int i = 0; i < rule; ++i) std::printf("-");
    std::printf("\n");
    for (const Entry& e : entries_) {
        const double speedup = base / e.ms;
        std::printf("%-*s %10.4f %12.1f %13.2fx%s\n",
                    static_cast<int>(nameW), e.name.c_str(), e.ms, e.mpix(), speedup,
                    (e.cut > 0.0 && e.ms > e.cut) ? "   <-- over budget"
                                                   : "");
    }
}

void BenchReport::printEntry(size_t i) const {
    if (i >= entries_.size()) return;
    const Entry& e = entries_[i];
    std::printf("%-*s %10.4f ms   %8.1f Mpix/s\n",
                static_cast<int>(std::max<size_t>(9, e.name.size())),
                e.name.c_str(), e.ms, e.mpix());
}

void BenchReport::printPairs() const {
    if (pairs_.empty()) return;

    size_t nameW = std::string("benchmark").size();
    for (const PairEntry& e : pairs_) nameW = std::max(nameW, e.name.size());

    std::printf("%-*s %12s %12s %12s %12s %10s\n",
                static_cast<int>(nameW), "benchmark",
                "serial ms", "serial Mpix/s", "parallel ms", "parallel Mpix/s",
                "speedup");
    const int rule = static_cast<int>(nameW) + 1 + 12 + 1 + 12 + 1 + 12 + 1 + 12 + 1 + 10;
    for (int i = 0; i < rule; ++i) std::printf("-");
    std::printf("\n");
    for (const PairEntry& e : pairs_) {
        std::printf("%-*s %12.4f %12.1f %12.4f %12.1f %9.2fx%s\n",
                    static_cast<int>(nameW), e.name.c_str(),
                    e.msSerial, e.mpixSerial(), e.msParallel, e.mpixParallel(),
                    e.speedup(),
                    (e.cut > 0.0 && e.msParallel > e.cut) ? "   <-- over budget"
                                                           : "");
    }
}

void fillGradient(Image& dst) {
    const uint32 w = dst.width(), h = dst.height();
    const DataType dt = dst.dataType();
    for (uint32 y = 0; y < h; ++y) {
        for (uint32 x = 0; x < w; ++x) {
            const float r = static_cast<float>(x) / static_cast<float>(w);
            const float g = static_cast<float>(y) / static_cast<float>(h);
            const float b = 0.5f;
            // 4-channel storage assumed below; single-channel keeps red ramp.
            switch (dst.channels()) {
                case 1:
                    if (dt == DataType::UInt8) dst.set<uint8>(x, y, 0, static_cast<uint8>(r * 255.f));
                    else                       dst.set<float>(x, y, 0, r);
                    break;
                default:
                    if (dt == DataType::UInt8) {
                        dst.set<uint8>(x, y, 0, static_cast<uint8>(r * 255.f));
                        dst.set<uint8>(x, y, 1, static_cast<uint8>(g * 255.f));
                        dst.set<uint8>(x, y, 2, static_cast<uint8>(b * 255.f));
                        if (dst.channels() == 4) dst.set<uint8>(x, y, 3, 255);
                    } else {
                        dst.set<float>(x, y, 0, r);
                        dst.set<float>(x, y, 1, g);
                        dst.set<float>(x, y, 2, b);
                        if (dst.channels() == 4) dst.set<float>(x, y, 3, 1.f);
                    }
            }
        }
    }
}

} // namespace bench
} // namespace iml