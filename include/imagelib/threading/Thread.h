#pragma once
#define IMAGELIB_THREADING_THREAD_H_
// imagelib/threading/Thread.h
//
// Minimal thread wrapper. ImageLib threads never own smart pointers; they are
// plain native threads with a thin, movable RAII surface so the rest of the
// threading layer (ThreadPool, ParallelFor) stays simple.

#include <thread>
#include <utility>

namespace iml {

class Thread {
public:
    Thread() noexcept = default;

    template <class Fn, class... Args>
    explicit Thread(Fn&& fn, Args&&... args);

    ~Thread();

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    Thread(Thread&& o) noexcept;
    Thread& operator=(Thread&& o) noexcept;

    void join();
    void detach();
    bool joinable() const noexcept;

    std::thread::id id() const noexcept;
    std::thread::native_handle_type nativeHandle() noexcept;

private:
    std::thread impl_;
};

/// Number of hardware threads, at least 1.
unsigned hardwareConcurrency() noexcept;

} // namespace iml

#include "imagelib/threading/Thread.tpp"