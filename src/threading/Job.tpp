#ifndef IMAGELIB_THREADING_JOB_H_
#error "Include Job.h, not Job.tpp directly."
#endif

#include <functional>
#include <type_traits>
#include <utility>

namespace iml {

/// Wraps any callable in the job.
template <class Fn, class> inline Job::Job(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}

} // namespace iml