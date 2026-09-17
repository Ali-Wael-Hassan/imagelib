#include "imagelib/benchmark/record/Entry.h"
#include "imagelib/benchmark/measure/Measure.h"

namespace iml {
namespace bench {

/// Throughput of this entry in Mega-pixels/second.
/// @return Mega-pixels processed per second.
double Entry::mpix() const noexcept { return mpixPerSec(pixels, ms); }

} // namespace bench
} // namespace iml