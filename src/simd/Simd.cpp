// imagelib/simd/Simd.cpp
//
// Runtime ISA detection and assured-feature resolution. Everything else in
// the SIMD tier is header-only and can be inlined into kernels.

#include "imagelib/simd/Simd.h"

namespace iml {
namespace simd {

namespace {

/// Read the OS XSAVE state bitmap. Bits 1|2 set means the OS preserves XMM/YMM
/// state (required for AVX); bits 4|5|6 set means ZMM state (AVX-512).
inline uint64 xgetbv0() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
    uint32 eax = 0, edx = 0;
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__(".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(0));
#endif
    return (static_cast<uint64>(edx) << 32) | eax;
#else
    return 0;
#endif
}

inline bool osSavesYmmState() noexcept {
    return (xgetbv0() & 0x6u) == 0x6u;
}

inline bool osSavesZmmState() noexcept {
    return (xgetbv0() & 0xE0u) == 0xE0u;
}

} // namespace

SimdFeatures cpuFeatures() noexcept {
#if defined(__x86_64__) || defined(__i386__)
#    if defined(__GNUC__) || defined(__clang__)
    SimdFeatures f(compileTimeFeatures());
    if (__builtin_cpu_supports("sse2"))    f |= SimdFeatures(Sse2);
    if (__builtin_cpu_supports("sse4.1"))  f |= SimdFeatures(Sse41);
    if (__builtin_cpu_supports("avx"))     f |= SimdFeatures(Avx);
    if (__builtin_cpu_supports("avx2"))    f |= SimdFeatures(Avx2);
    if (__builtin_cpu_supports("avx512f")) f |= SimdFeatures(Avx512F);
    return f;
#    else
    return compileTimeFeatures();
#    endif
#else
    return compileTimeFeatures();
#endif
}

SimdFeatures assuredFeatures() noexcept {
    SimdFeatures f = compileTimeFeatures();
    if (f.empty()) return f;
    f &= cpuFeatures();
    if (!osSavesYmmState()) {
        f.mask &= ~(uint64(Avx) | uint64(Avx2) | uint64(Avx512F));
    } else if (!osSavesZmmState()) {
        f.mask &= ~uint64(Avx512F);
    }
    return f;
}

size_t vectorBytes() noexcept {
    const SimdFeatures f = assuredFeatures();
    if (f.has(Avx512F)) return 64;
    if (f.has(Avx2) || f.has(Avx)) return 32;
    if (f.has(Sse2) || f.has(Neon) || f.has(SVE)) return 16;
    return 0;
}

const char* activeIsaName() noexcept {
    const SimdFeatures f = assuredFeatures();
    if (f.has(Avx512F)) return "AVX512F";
    if (f.has(Avx2))    return "AVX2";
    if (f.has(Avx))     return "AVX";
    if (f.has(Neon))    return "NEON";
    if (f.has(Sse2))    return "SSE2";
    if (f.has(SVE))     return "SVE";
    return "scalar";
}

} // namespace simd
} // namespace iml