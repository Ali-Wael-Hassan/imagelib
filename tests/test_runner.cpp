// tests/test_runner.cpp
#include "tests.h"

TEST_RUNNER_FORWARD(types)
TEST_RUNNER_FORWARD(memory)
TEST_RUNNER_FORWARD(math)
TEST_RUNNER_FORWARD(image)
TEST_RUNNER_FORWARD(compression)
TEST_RUNNER_FORWARD(simd)
TEST_RUNNER_FORWARD(threading)
TEST_RUNNER_FORWARD(processing)
TEST_RUNNER_FORWARD(procedural)
TEST_RUNNER_FORWARD(codecs)

int main() {
    struct Group { const char* name; bool (*fn)(); };
    const Group groups[] = {
        { "types",       test_types       },
        { "memory",      test_memory      },
        { "math",        test_math        },
        { "image",       test_image       },
        { "compression", test_compression },
        { "simd",        test_simd        },
        { "threading",   test_threading   },
        { "processing",  test_processing  },
        { "procedural",  test_procedural  },
        { "codecs",      test_codecs      },
    };
    int totalFailed = 0;
    std::printf("ImageLib test suite\n");
    for (const auto& g : groups) {
        const int before = imltest::failures();
        g.fn();
        const int n = imltest::failures() - before;
        totalFailed += n;
        if (n == 0) std::printf("[OK  ] %-12s\n", g.name);
        else        std::printf("[FAIL] %-12s (%d failures)\n", g.name, n);
    }
    std::printf(totalFailed == 0 ? "\nALL TESTS PASSED\n" : "\nTESTS FAILED (%d failures)\n", totalFailed);
    return totalFailed == 0 ? 0 : 1;
}