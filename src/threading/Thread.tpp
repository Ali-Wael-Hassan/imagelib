#ifndef IMAGELIB_THREADING_THREAD_H_
#error "Include Thread.h, not Thread.tpp directly."
#endif

#include <thread>
#include <utility>

namespace iml {

/// Starts a thread running fn with the given arguments.
template <class Fn, class... Args>
inline Thread::Thread(Fn&& fn, Args&&... args)
    : impl_(std::forward<Fn>(fn), std::forward<Args>(args)...) {}

} // namespace iml