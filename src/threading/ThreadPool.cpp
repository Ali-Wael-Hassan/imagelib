#include "imagelib/threading/ThreadPool.h"

#include <stdexcept>

namespace iml {

/// Constructs and starts the pool.
ThreadPool::ThreadPool(size_t workers) { start(workers); }

/// Stops and joins all workers.
ThreadPool::~ThreadPool() { stop(); }

/// Starts (or restarts) the pool with the given worker count.
void ThreadPool::start(size_t workers) {
    stop();

    if (workers == 0) {
        const unsigned hw = hardwareConcurrency();
        workers = hw > 1 ? static_cast<size_t>(hw - 1) : 1u;
    }
    if (workers == 0)
        workers = 1;

    {
        std::unique_lock<std::mutex> lock(mtx_);
        queue_.clear();
        pending_ = 0;
        stopping_ = false;
    }
    threads_.reserve(workers);
    for (size_t i = 0; i < workers; ++i) {
        threads_.emplace_back(&ThreadPool::workerLoop, this);
    }
}

/// Stops the pool, dropping queued jobs and joining workers.
void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (threads_.empty()) {
            stopping_ = false;
            queue_.clear();
            return;
        }
        queue_.clear();
        stopping_ = true;
    }
    cvWork_.notify_all();
    for (Thread& t : threads_) {
        if (t.joinable())
            t.join();
    }
    threads_.clear();
    {
        std::unique_lock<std::mutex> lock(mtx_);
        stopping_ = false;
        pending_ = 0;
    }
}

/// Non-blocking enqueue of a job.
/// @return True when the job was accepted.
bool ThreadPool::tryPush(Job&& job) {
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (stopping_ || threads_.empty())
            return false;
        queue_.push_back(std::move(job));
    }
    cvWork_.notify_one();
    return true;
}

/// Blocking enqueue of a job.
/// @throws std::runtime_error When the pool is not running.
void ThreadPool::push(Job&& job) {
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (stopping_ || threads_.empty()) {
            throw std::runtime_error("ThreadPool::push: pool is not running");
        }
        queue_.push_back(std::move(job));
    }
    cvWork_.notify_one();
}

/// Waits until the queue is empty and all workers are idle.
void ThreadPool::waitAll() {
    std::unique_lock<std::mutex> lock(mtx_);
    cvIdle_.wait(lock, [this] { return queue_.empty() && pending_ == 0; });
}

/// Number of jobs currently queued.
size_t ThreadPool::queueSize() const noexcept {
    std::unique_lock<std::mutex> lock(mtx_);
    return queue_.size();
}

/// Number of live worker threads.
size_t ThreadPool::workerCount() const noexcept { return threads_.size(); }

/// True while the pool is running.
bool ThreadPool::running() const noexcept { return !threads_.empty() && !stopping_; }

/// Worker thread entry point: pulls and runs jobs until stopped.
void ThreadPool::workerLoop() noexcept {
    for (;;) {
        Job job;
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cvWork_.wait(lock, [this] { return !queue_.empty() || stopping_; });
            if (queue_.empty() && stopping_)
                break;
            job = std::move(queue_.front());
            queue_.pop_front();
            ++pending_;
        }
        job.run();
        job.reset();
        {
            std::unique_lock<std::mutex> lock(mtx_);
            --pending_;
        }
        cvIdle_.notify_all();
    }
}

/// Returns the process-wide default worker pool.
ThreadPool& defaultThreadPool() noexcept {
    static ThreadPool pool;
    return pool;
}

} // namespace iml