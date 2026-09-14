// tests/test_simd.cpp – SIMD detection, dispatch, splitting, reference kernels
#include "tests.h"
#include "imagelib/simd/Simd.h"
#include "imagelib/core/ExecutionPolicy.h"
#include <cstring>
#include <cstdlib>

using namespace iml;

static uint8_t refAvg(uint8_t a, uint8_t b) { return static_cast<uint8_t>((a + b + 1u) >> 1); }
static uint8_t refSat(uint8_t a, uint8_t b) {
    unsigned v = static_cast<unsigned>(a) + b;
    return v > 255u ? 255u : static_cast<uint8_t>(v);
}

bool test_simd() {
    // feature masks
    simd::SimdFeatures ct = simd::compileTimeFeatures();
#if defined(__x86_64__) || defined(_M_X64)
    TEST_CHECK(ct.has(simd::Sse2));
#endif
    TEST_CHECK(ct == simd::SimdFeatures(ct.mask));
    simd::SimdFeatures cpu = simd::cpuFeatures();
    TEST_CHECK(!cpu.empty());

    simd::SimdFeatures assured = simd::assuredFeatures();
    const bool av = simd::simdAvailable();
    TEST_CHECK(av == assured.has(simd::Sse2) || assured.has(simd::Neon) ||
               assured.has(simd::Avx2) || assured.has(simd::Avx512F) || assured.has(simd::SVE));

    // widths
    const size_t bytes = simd::vectorBytes();
    TEST_CHECK(bytes > 0 && (bytes & (bytes - 1)) == 0);
    TEST_CHECK(simd::simdWidth<uint8_t>() == bytes);
    TEST_CHECK(simd::simdWidth<float>() == bytes / sizeof(float));
    TEST_CHECK(simd::simdWidth<uint16_t>() == bytes / 2);
    TEST_CHECK(simd::simdWidth<double>() == bytes / 8);

    // dispatch
    TEST_CHECK(simd::dispatchMode(ExecutionPolicy::serial(), 100000) == simd::DispatchMode::Scalar);
    if (av) {
        TEST_CHECK(simd::dispatchMode(ExecutionPolicy::simd(), 4) == simd::DispatchMode::Simd);
        TEST_CHECK(simd::dispatchMode(ExecutionPolicy(), 100000) == simd::DispatchMode::Simd); // Auto
        TEST_CHECK(simd::dispatchMode(ExecutionPolicy(), 4) == simd::DispatchMode::Scalar);    // tiny
    } else {
        TEST_CHECK(simd::dispatchMode(ExecutionPolicy::simd(), 4) == simd::DispatchMode::Scalar);
    }

    // splitting
    TEST_CHECK(simd::split<uint8_t>(0).headBlocks == 0 && simd::split<uint8_t>(0).vectorElems == 0);
    {
        const size_t lanes = simd::simdWidth<uint8_t>();
        simd::SimdSplit sp = simd::split<uint8_t>(100);
        TEST_CHECK(sp.headBlocks == 0);
        TEST_CHECK(sp.vectorElems == lanes * (100 / lanes));
        TEST_CHECK(sp.tail == 100 % lanes);
        TEST_CHECK(sp.headBlocks + sp.vectorElems + sp.tail == 100);
    }
    {
        std::uint8_t buf[2 * sizeof(size_t) + 2] = {0};
        const std::uint8_t* base = buf + 1;
        simd::SimdSplit sp = simd::split<uint8_t>(100, base);
        TEST_CHECK(sp.headBlocks < bytes); // scrapes alignment within one vector
        TEST_CHECK(sp.headBlocks + sp.vectorElems + sp.tail == 100);
        uintptr_t vecStart = reinterpret_cast<uintptr_t>(base) + sp.headBlocks;
        TEST_CHECK((vecStart & (bytes - 1)) == 0);
    }

    // kernels vs scalar reference
    {
        std::uint8_t a[1000], b[1000], ref[1000], out[1000];
        for (size_t i = 0; i < 1000; ++i) {
            a[i] = static_cast<std::uint8_t>(std::rand() & 0xFF);
            b[i] = static_cast<std::uint8_t>(std::rand() & 0xFF);
            ref[i] = refAvg(a[i], b[i]);
        }
        simd::averageU8(out, a, b, 1000);
        TEST_CHECK(std::memcmp(out, ref, 1000) == 0);
        for (size_t i = 0; i < 1000; ++i) ref[i] = refSat(a[i], b[i]);
        simd::saturatingAddU8(out, a, b, 1000);
        TEST_CHECK(std::memcmp(out, ref, 1000) == 0);
    }
    {
        // odd sizes exercise the scalar tail path
        std::uint8_t a[999], b[999], ref[999], out[999];
        for (size_t i = 0; i < 999; ++i) {
            a[i] = static_cast<std::uint8_t>((i * 7 + 3) & 0xFF);
            b[i] = static_cast<std::uint8_t>((i * 13) & 0xFF);
            ref[i] = refAvg(a[i], b[i]);
        }
        simd::averageU8(out, a, b, 999);
        TEST_CHECK(std::memcmp(out, ref, 999) == 0);
        for (size_t i = 0; i < 999; ++i) ref[i] = refSat(a[i], b[i]);
        simd::saturatingAddU8(out, a, b, 999);
        TEST_CHECK(std::memcmp(out, ref, 999) == 0);

        simd::averageU8(out, a, b, 1);
        TEST_CHECK(out[0] == refAvg(a[0], b[0]));
    }

    return true;
}