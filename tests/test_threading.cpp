// tests/test_threading.cpp – Thread, Job, ThreadPool, ParallelFor
#include "tests.h"
#include "imagelib/threading/Thread.h"
#include "imagelib/threading/Job.h"
#include "imagelib/threading/ThreadPool.h"
#include "imagelib/threading/ParallelFor.h"
#include "imagelib/core/ExecutionPolicy.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <stdexcept>

using namespace iml;

static void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

static long long runTaskSum(int n, const ExecutionPolicy& policy) {
    long long out = 0;
    std::mutex m;
    parallelFor(0, (size_t)n, [&](size_t i) {
        std::lock_guard<std::mutex> g(m);
        out += static_cast<long long>(i);
    }, policy);
    return out;
}

bool test_threading() {
    // Thread
    {
        std::atomic<int> value{0};
        Thread t([](std::atomic<int>* v) { v->store(7); }, &value);
        TEST_CHECK(t.joinable());
        t.join();
        TEST_CHECK(!t.joinable());
        TEST_CHECK(value.load() == 7);
    }
    {
        Thread t([]() { sleepMs(5); });
        t.join();
    }

    // Job
    {
        Job j([]() {});
        TEST_CHECK(static_cast<bool>(j));
        int ran = 0;
        Job j2([&ran]() { ran = 1; });
        j2.run();
        TEST_CHECK(ran == 1);
        Job empty;
        TEST_CHECK(!static_cast<bool>(empty));
        empty.run(); // no-op, must not crash
        empty.reset();
    }

    // ThreadPool
    {
        ThreadPool pool(2);
        TEST_CHECK(pool.workerCount() == 2);
        TEST_CHECK(pool.running());

        std::atomic<long long> sum{0};
        for (int i = 0; i < 200; ++i) {
            pool.tryPush(Job([&sum, i]() { sum += i; }));
        }
        pool.waitAll();
        TEST_CHECK(sum == 19900); // 0+1+...+199
        TEST_CHECK(pool.queueSize() == 0);

        // idle between batches (quiesce works repeatedly)
        std::atomic<long long> sum2{0};
        for (int i = 0; i < 10; ++i) pool.tryPush(Job([&sum2, i]() { sum2 += i; }));
        pool.waitAll();
        TEST_CHECK(sum2 == 45);

        pool.stop();
        TEST_CHECK(!pool.running());

        // restart
        pool.start(2);
        std::atomic<int> v{0};
        pool.push(Job([&v]() { v = 3; }));
        pool.waitAll();
        TEST_CHECK(v == 3);

        // tryPush on a stopped pool -> false; push -> throws
        pool.stop();
        TEST_CHECK(!pool.tryPush(Job([]() {})));
        bool threw = false;
        try { pool.push(Job([]() {})); } catch (const std::runtime_error&) { threw = true; }
        TEST_CHECK(threw);
    }

    // ParallelFor
    {
        const long long expect = 49995000LL;      // sum 0..9999
        const long long expectM = 1999999000000LL; // sum 0..1999999
        TEST_CHECK(runTaskSum(10000, ExecutionPolicy::serial()) == expect);
        ExecutionPolicy par;
        par.mode = ExecutionMode::Parallel;
        par.maxWorkers = 4;
        TEST_CHECK(runTaskSum(10000, par) == expect);
        ExecutionPolicy autoP;
        TEST_CHECK(runTaskSum(2000000, autoP) == expectM); // Auto, large -> pool
        TEST_CHECK(runTaskSum(0, autoP) == 0);
        TEST_CHECK(runTaskSum(1, autoP) == 0);
    }

    {
        // parallelForRows : independent row writes
        std::atomic<int> rows[64];
        parallelForRows(64, [&](size_t r) { rows[r] = static_cast<int>(r); });
        int bad = 0;
        for (int r = 0; r < 64; ++r) if (rows[r] != r) ++bad;
        TEST_CHECK(bad == 0);
    }

    {
        // parallelForColumns : independent column writes (strided)
        std::atomic<int> cols[64];
        parallelForColumns(64, [&](size_t c) { cols[c] = static_cast<int>(c); });
        int bad = 0;
        for (int c = 0; c < 64; ++c) if (cols[c] != c) ++bad;
        TEST_CHECK(bad == 0);
    }

    {
        // per-index exactly-once guarantee under threads
        ThreadPool pool(4);
        std::atomic<int> counts[1000];
        for (int i = 0; i < 1000; ++i) counts[i] = 0;
        for (int chunk = 0; chunk < 5; ++chunk) {
            for (int i = 0; i < 1000; ++i) {
                pool.tryPush(Job([&counts, i]() { counts[i].fetch_add(1); }));
            }
            pool.waitAll();
            int missing = 0;
            for (int i = 0; i < 1000; ++i) if (counts[i].load() != chunk + 1) ++missing;
            TEST_CHECK(missing == 0);
        }
    }

    return true;
}