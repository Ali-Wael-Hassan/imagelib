#pragma once
#define IMAGELIB_THREADING_JOB_H_
/// @file Job.h
/// A unit of deferred work consumed by ThreadPool workers. Holds any
/// callable via type erasure; jobs are created once and moved, never copied.

#include <functional>
#include <utility>

namespace iml {

/// A unit of deferred work holding any callable via type erasure.
class Job {
  public:
    Job() noexcept = default;

    /// Constructs a job wrapping any callable.
    /// @tparam Fn Callable type.
    template <class Fn, class = std::enable_if_t<!std::is_same<Job, std::decay_t<Fn>>::value>>
    explicit Job(Fn&& fn);

    Job(Job&&) noexcept = default;
    Job& operator=(Job&&) noexcept = default;
    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;

    /// Invokes the stored callable; a no-op when empty.
    void run() const;
    /// True when the job holds a callable.
    explicit operator bool() const noexcept;
    /// Drops the stored callable, leaving the job empty.
    void reset() noexcept;

  private:
    std::function<void()> fn_;
};

} // namespace iml

#include "threading/Job.tpp"