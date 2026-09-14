// imagelib/src/threading/Thread.cpp
//
// Out-of-line non-template member definitions moved out of Thread.h.

#include "imagelib/threading/Thread.h"

#include <thread>

namespace iml {

Thread::~Thread() {
    if (impl_.joinable()) impl_.join();
}

Thread::Thread(Thread&& o) noexcept : impl_(std::move(o.impl_)) {}

Thread& Thread::operator=(Thread&& o) noexcept {
    if (this != &o) {
        if (impl_.joinable()) impl_.join();
        impl_ = std::move(o.impl_);
    }
    return *this;
}

void Thread::join() { impl_.join(); }
void Thread::detach() { impl_.detach(); }
bool Thread::joinable() const noexcept { return impl_.joinable(); }

std::thread::id Thread::id() const noexcept { return impl_.get_id(); }
std::thread::native_handle_type Thread::nativeHandle() noexcept { return impl_.native_handle(); }

unsigned hardwareConcurrency() noexcept {
    const unsigned hw = std::thread::hardware_concurrency();
    return hw == 0 ? 1 : hw;
}

} // namespace iml