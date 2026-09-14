#pragma once
// tests/tests.h
//
// Minimal dependency-free test harness for the ImageLib test suite. Each
// test_*.cpp includes this header, defines TEST_CHECK/TEST_CHECK_NEAR, and
// publishes a suite function via IML_TEST_SUITE. test_runner.cpp calls every
// suite and reports the grand total.

#include <cstdio>
#include <cmath>

namespace imltest {

/// Total failed assertions across all suites.
inline int& failures() {
    static int f = 0;
    return f;
}

inline void report(bool ok, const char* expr, const char* file, int line) {
    if (!ok) {
        ++failures();
        std::printf("    FAIL %s:%d  %s\n", file, line, expr);
    }
}

inline void reportNear(double a, double b, double eps, const char* ea,
                       const char* eb, const char* file, int line) {
    if (!(std::fabs(a - b) <= eps)) {
        ++failures();
        std::printf("    FAIL %s:%d  %s ~= %s  (%.9g vs %.9g)\n", file, line, ea, eb, a, b);
    }
}

inline int summary(const char* group) {
    const int n = failures();
    if (n == 0) std::printf("[OK]   %s\n", group);
    else        std::printf("[FAIL] %s (%d failures)\n", group, n);
    return n;
}

} // namespace imltest

#define TEST_CHECK(cond) ::imltest::report((cond), #cond, __FILE__, __LINE__)
#define TEST_CHECK_NEAR(a, b, eps) ::imltest::reportNear((a), (b), (eps), #a, #b, __FILE__, __LINE__)

/// Define a test suite: IML_TEST_SUITE(name, body) -> `bool test_##name()`.
#define IML_TEST_SUITE(name)                       \
    bool test_##name()

#define TEST_RUNNER_FORWARD(name) bool test_##name();