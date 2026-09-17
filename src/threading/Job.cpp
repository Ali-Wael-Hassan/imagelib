#include "imagelib/threading/Job.h"

namespace iml {

/// Invokes the stored callable; a no-op when empty.
void Job::run() const {
    if (fn_)
        fn_();
}

/// True when the job holds a callable.
Job::operator bool() const noexcept { return static_cast<bool>(fn_); }

/// Drops the stored callable, leaving the job empty.
void Job::reset() noexcept { fn_ = {}; }

} // namespace iml