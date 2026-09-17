#include "imagelib/benchmark/record/PairEntry.h"
#include "imagelib/benchmark/measure/Measure.h"

namespace iml {
namespace bench {

/// Serial throughput in Mega-pixels/second.
/// @return Mega-pixels processed per second.
double PairEntry::mpixSerial() const noexcept { return mpixPerSec(pixels, msSerial); }

/// Parallel throughput in Mega-pixels/second.
/// @return Mega-pixels processed per second.
double PairEntry::mpixParallel() const noexcept { return mpixPerSec(pixels, msParallel); }

/// Parallel speedup over the serial baseline (1.0 = no gain).
/// @return Serial time divided by parallel time, else 1.0.
double PairEntry::speedup() const noexcept {
    return msSerial > 0.0 ? msSerial / (msParallel > 0.0 ? msParallel : 1e-9) : 1.0;
}

} // namespace bench
} // namespace iml