// examples/example_util.h
//
// Shared helpers for the ImageLib example executables: argument/policy parsing,
// image load/save, and timing output. Every example runs its kernel with
// ExecutionPolicy::simdParallel() by default (SIMD + thread pool + auto
// serial fallback for tiny inputs). Pass "serial" as the last argument to
// compare against the single-threaded path.

#pragma once

#include "imagelib/imagelib.h"
#include "imagelib/benchmark/Benchmark.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>

namespace iml_example {

/// Default high-performance policy used by all examples.
inline iml::ExecutionPolicy defaultPolicy(bool serial = false) noexcept {
    return serial ? iml::ExecutionPolicy::serial()
                  : iml::ExecutionPolicy::simdParallel();
}

/// Nice text describing a policy, for the banner.
inline const char* policyName(bool serial) noexcept {
    return serial ? "serial" : "simd + parallel";
}

/// True when `argv[argc-1] == "serial"`.
inline bool wantsSerial(int argc, char** argv) {
    return argc >= 2 && std::string(argv[argc - 1]) == "serial";
}

/// Loads an image or exits with a usage message.
inline iml::Image loadOrDie(const std::string& path) {
    try {
        iml::Image img(path);
        return img;
    } catch (const iml::Error& e) {
        std::printf("error: could not load '%s': %s\n", path.c_str(), e.what());
        std::exit(1);
    }
}

/// Saves an image or exits with an error message.
inline void saveOrDie(const iml::Image& img, const std::string& path) {
    try {
        img.save(path);
    } catch (const iml::Error& e) {
        std::printf("error: could not save '%s': %s\n", path.c_str(), e.what());
        std::exit(1);
    }
}

/// Times `fn` through the shared benchmark utility (best of `iterations` runs
/// after `warmup` warm-ups), prints "name: X.XX ms (Y.Y Mpix/s)" and returns
/// the best time in ms. `pixels` is used for the throughput column.
template <class Fn>
inline double timeAndReport(const char* name, std::uint64_t pixels, Fn&& fn,
                            int iterations = 3, int warmup = 1) {
    const double ms = iml::bench::bestMs(iterations, warmup, std::forward<Fn>(fn));
    const double mpix = iml::bench::mpixPerSec(pixels, ms);
    std::printf("  %-34s %8.2f ms   %8.1f Mpix/s\n", name, ms, mpix);
    return ms;
}

/// Rows for the self-formatting comparison table from the benchmark utility.
inline void banner(const char* title, const iml::Image& img) {
    std::printf("== %s ==\n", title);
    std::printf("  input %ux%u, %u channel(s)\n",
                img.width(), img.height(), static_cast<unsigned>(img.channels()));
}

} // namespace iml_example