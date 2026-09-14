#pragma once
#define IMAGELIB_THREADING_JOB_H_
// imagelib/threading/Job.h
//
// A unit of deferred work consumed by ThreadPool workers. Holds any callable
// via type erasure; jobs are created once and moved, never copied, so the
// capture payload can be a non-trivial object (e.g. an array of temporaries).

#include <functional>
#include <utility>

namespace iml {

class Job {
public:
    Job() noexcept = default;

    template <class Fn, class = std::enable_if_t<!std::is_same<Job, std::decay_t<Fn>>::value>>
    explicit Job(Fn&& fn);

    Job(Job&&) noexcept = default;
    Job& operator=(Job&&) noexcept = default;
    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;

    void run() const;
    explicit operator bool() const noexcept;
    void reset() noexcept;

private:
    std::function<void()> fn_;
};

} // namespace iml

#include "imagelib/threading/Job.tpp"