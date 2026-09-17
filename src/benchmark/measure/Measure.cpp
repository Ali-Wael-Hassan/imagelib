#include "imagelib/benchmark/measure/Measure.h"

namespace iml {
namespace bench {

/// Reads the steady clock in milliseconds.
/// @return Current time in milliseconds.
double nowMs() noexcept {
    return std::chrono::duration<double, std::milli>(Clock::now().time_since_epoch()).count();
}

/// Throughput of `pixels` processed in `ms`, in Mega-pixels/second.
/// @param pixels Number of pixels processed.
/// @param ms Elapsed time in milliseconds.
/// @return Mega-pixels processed per second.
double mpixPerSec(uint64 pixels, double ms) noexcept {
    return static_cast<double>(pixels) / (ms * 1000.0);
}

} // namespace bench
} // namespace iml