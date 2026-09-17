#pragma once
#define IMAGELIB_THREADING_THREADPOOL_H_
/// @file ThreadPool.h
/// Fixed-size worker pool: callers push Jobs, workers pull them off a shared
/// queue. Jobs must not call back into the pool's waitAll()/stop().

#include "imagelib/threading/Thread.h"
#include "imagelib/threading/Job.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

namespace iml {

/// Fixed-size worker pool executing Jobs from a shared queue.
class ThreadPool {
  public:
    /// Constructs and starts a pool with `workers` threads (0 => hardware
    /// concurrency minus one, at least one).
    explicit ThreadPool(size_t workers = 0);

    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// (Re)starts the pool with the given worker count. Any previously
    /// running pool is stopped first.
    void start(size_t workers = 0);

    /// Stops the pool: signals workers to exit and joins them. Jobs still
    /// queued are dropped. Does not wait for the last batch to complete.
    void stop();

    /// Enqueues a job without blocking. Returns false when the pool is not
    /// running (stopped or never started).
    bool tryPush(Job&& job);

    /// Enqueues a job, blocking until it is accepted (returns immediately
    /// unless the pool is stopping shortly after start). Throws
    /// std::runtime_error when the pool is not running.
    void push(Job&& job);

    /// Waits until the queue is empty AND all workers are idle ("quiesce").
    void waitAll();

    /// Number of live worker threads.
    size_t workerCount() const noexcept;

    /// Number of jobs currently queued (not yet picked up).
    size_t queueSize() const noexcept;

    /// True while the pool is running (started and not stopped).
    bool running() const noexcept;

  private:
    void workerLoop() noexcept;

    std::vector<Thread> threads_;
    std::deque<Job> queue_;
    mutable std::mutex mtx_;
    std::condition_variable cvWork_;
    std::condition_variable cvIdle_;
    size_t pending_ = 0;
    bool stopping_ = false;
};

/// Process-wide default worker pool (started lazily on first use).
ThreadPool& defaultThreadPool() noexcept;

} // namespace iml