// imagelib/threading/Thread.tpp
//
// Template members of Thread.h. This file is included at the bottom of
// Thread.h (never include it directly).

#ifndef IMAGELIB_THREADING_THREAD_H_
#error "Include Thread.h, not Thread.tpp directly."
#endif

#include <thread>
#include <utility>

namespace iml {

template <class Fn, class... Args>
inline Thread::Thread(Fn&& fn, Args&&... args)
    : impl_(std::forward<Fn>(fn), std::forward<Args>(args)...) {}

} // namespace iml