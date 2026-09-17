#include "imagelib/threading/Thread.h"

#include <thread>

namespace iml {

/// Destructor; joins the thread if still joinable.
Thread::~Thread() {
    if (impl_.joinable())
        impl_.join();
}

/// Move constructor taking over another thread's execution.
Thread::Thread(Thread&& o) noexcept : impl_(std::move(o.impl_)) {}

/// Move assignment, joining the current thread first.
Thread& Thread::operator=(Thread&& o) noexcept {
    if (this != &o) {
        if (impl_.joinable())
            impl_.join();
        impl_ = std::move(o.impl_);
    }
    return *this;
}

/// Waits for the thread to finish.
void Thread::join() { impl_.join(); }
/// Detaches the thread; it continues running independently.
void Thread::detach() { impl_.detach(); }
/// True when an execution thread is associated.
bool Thread::joinable() const noexcept { return impl_.joinable(); }

/// Returns the thread's identifier.
std::thread::id Thread::id() const noexcept { return impl_.get_id(); }
/// Returns the implementation-defined native handle.
std::thread::native_handle_type Thread::nativeHandle() noexcept { return impl_.native_handle(); }

/// Number of hardware threads, at least 1.
unsigned hardwareConcurrency() noexcept {
    const unsigned hw = std::thread::hardware_concurrency();
    return hw == 0 ? 1 : hw;
}

} // namespace iml