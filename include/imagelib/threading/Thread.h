#pragma once
#define IMAGELIB_THREADING_THREAD_H_
/// @file Thread.h
/// Minimal movable RAII wrapper around a native thread.

#include <thread>
#include <utility>

namespace iml {

/// Minimal movable RAII wrapper around a native thread.
class Thread {
  public:
    Thread() noexcept = default;

    /// Starts a thread running fn with the given arguments.
    /// @tparam Fn Callable type.
    /// @tparam Args Argument types forwarded to fn.
    template <class Fn, class... Args> explicit Thread(Fn&& fn, Args&&... args);

    /// Destructor; joins the thread if still joinable.
    ~Thread();

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    /// Move constructor taking over another thread's execution.
    Thread(Thread&& o) noexcept;
    /// Move assignment, joining the current thread first.
    Thread& operator=(Thread&& o) noexcept;

    /// Waits for the thread to finish.
    void join();
    /// Detaches the thread; it continues running independently.
    void detach();
    /// True when an execution thread is associated.
    bool joinable() const noexcept;

    /// Returns the thread's identifier.
    std::thread::id id() const noexcept;
    /// Returns the implementation-defined native handle.
    std::thread::native_handle_type nativeHandle() noexcept;

  private:
    std::thread impl_;
};

/// Number of hardware threads, at least 1.
unsigned hardwareConcurrency() noexcept;

} // namespace iml

#include "threading/Thread.tpp"