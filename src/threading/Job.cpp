// imagelib/src/threading/Job.cpp
//
// Out-of-line non-template member definitions moved out of Job.h.

#include "imagelib/threading/Job.h"

namespace iml {

void Job::run() const {
    if (fn_) fn_();
}

Job::operator bool() const noexcept {
    return static_cast<bool>(fn_);
}

void Job::reset() noexcept {
    fn_ = {};
}

} // namespace iml