#pragma once
// imagelib/simd/Simd.h
//
// SIMD and execution dispatch primitives.
//
// ImageLib's algorithm hierarchy is Scalar -> SIMD -> Multithreaded
// -> SIMD + Multithreaded. This header provides:
//   - compile-time and runtime ISA feature detection
//   - a minimal, portable, 128-bit vector tier (SSE2 / NEON) plus optional
//     higher tiers guarded at runtime
//   - simdWidth<T>() and a work-load splitter so kernels can process whole
//     vectors then a scalar tail
//   - a couple of reference kernels used by the self-check; processing
//     algorithms build their own kernels on top of these primitives
//
// x86-64 guarantees SSE2 and AArch64 guarantees NEON, so the 128-bit tier is
// available in the default portable build. Higher ISAs are only used when
// IML_ENABLE_NATIVE is defined and the CPU actually supports them.

#include "imagelib/core/Types.h"
#include "imagelib/core/ExecutionPolicy.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

// ---------------------------------------------------------------------------
// Compiler / ISA macros.
// ---------------------------------------------------------------------------

#if defined(__GNUC__) || defined(__clang__)
#    define IML_SIMD_ALWAYS_INLINE inline __attribute__((always_inline))
#    define IML_SIMD_RESTRICT __restrict__
#else
#    define IML_SIMD_ALWAYS_INLINE inline
#    define IML_SIMD_RESTRICT
#endif

#if (defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__)) && !defined(__SSE2__)
#    define IML_SIMD_SSE2_FORCED
#else
#    define IML_SIMD_SSE2_FORCED 0
#endif

#if defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__)
#    define IML_SIMD_SSE2 1
#else
#    define IML_SIMD_SSE2 0
#endif

#if defined(__SSE4_1__)
#    define IML_SIMD_SSE41 1
#else
#    define IML_SIMD_SSE41 0
#endif

#if defined(__AVX2__)
#    define IML_SIMD_AVX2 1
#else
#    define IML_SIMD_AVX2 0
#endif

#if defined(__AVX512F__)
#    define IML_SIMD_AVX512F 1
#else
#    define IML_SIMD_AVX512F 0
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
#    define IML_SIMD_NEON 1
#else
#    define IML_SIMD_NEON 0
#endif

#if IML_SIMD_SSE2
#    include <emmintrin.h>
#endif
#if IML_SIMD_SSE41
#    include <smmintrin.h>
#endif
#if IML_SIMD_AVX2
#    include <immintrin.h>
#endif
#if IML_SIMD_NEON
#    include <arm_neon.h>
#endif

namespace iml {
namespace simd {

/// Bit-flag SIMD ISA features. The auditable mask is a plain uint64 so it can
/// be inspected, serialized and combined without extra machinery.
enum SimdFeature : uint64 {
    Sse2       = 1ull << 0,
    Sse41      = 1ull << 1,
    Avx        = 1ull << 2,
    Avx2       = 1ull << 3,
    Avx512F    = 1ull << 4,
    Neon       = 1ull << 5,
    SVE        = 1ull << 6,
};

/// Set of SIMD features represented as a bitmask.
struct SimdFeatures {
    uint64 mask = 0;

    constexpr SimdFeatures() noexcept = default;
    constexpr explicit SimdFeatures(uint64 m) noexcept : mask(m) {}
    constexpr explicit SimdFeatures(SimdFeature f) noexcept : mask(static_cast<uint64>(f)) {}

    constexpr bool has(SimdFeature f) const noexcept { return (mask & static_cast<uint64>(f)) != 0; }
    constexpr bool empty() const noexcept { return mask == 0; }

    constexpr SimdFeatures operator|(const SimdFeatures& o) const noexcept {
        return SimdFeatures(mask | o.mask);
    }
    constexpr SimdFeatures operator&(const SimdFeatures& o) const noexcept {
        return SimdFeatures(mask & o.mask);
    }
    SimdFeatures& operator|=(const SimdFeatures& o) noexcept { mask |= o.mask; return *this; }
    SimdFeatures& operator&=(const SimdFeatures& o) noexcept { mask &= o.mask; return *this; }

    constexpr bool operator==(const SimdFeatures& o) const noexcept { return mask == o.mask; }
    constexpr bool operator!=(const SimdFeatures& o) const noexcept { return mask != o.mask; }
};

/// ISA features the compiler was built for (intrinsics available without
/// runtime guarding).
constexpr SimdFeatures compileTimeFeatures() noexcept {
    uint64 m = 0;
    if (IML_SIMD_SSE2)    m |= uint64(Sse2);
    if (IML_SIMD_SSE41)   m |= uint64(Sse41);
    if (IML_SIMD_AVX2)    m |= uint64(Avx2);
    if (IML_SIMD_AVX512F) m |= uint64(Avx512F);
    if (IML_SIMD_NEON)    m |= uint64(Neon);
    return SimdFeatures(m);
}

/// ISA features reported by the CPU at runtime (independent of what the
/// compiler can emit). Implemented in src/simd/Simd.cpp.
SimdFeatures cpuFeatures() noexcept;

/// Features we are safe to actually execute: compiled in AND supported by the
/// CPU. On x86-64 the SSE2 tier always survives; higher tiers are gated on the
/// detected hardware.
SimdFeatures assuredFeatures() noexcept;

/// True when at least one vector ISA is available (128-bit tier on x86-64 /
/// AArch64, larger tiers when enabled and detected).
inline bool simdAvailable() noexcept {
    return assuredFeatures().has(Sse2) || assuredFeatures().has(Neon)
        || assuredFeatures().has(Avx2) || assuredFeatures().has(Avx512F)
        || assuredFeatures().has(SVE);
}

/// Bytes processed per vector operation. The 128-bit baseline is always
/// available on 64-bit x86/ARM targets; larger widths are only selected when
/// the ISA is both compiled in and detected at runtime.
size_t vectorBytes() noexcept;

/// Short human-readable description of the active ISA ("SSE2", "AVX2", ...).
const char* activeIsaName() noexcept;

// ---------------------------------------------------------------------------
// Vectorizable element types.
// ---------------------------------------------------------------------------

template <class T>
struct isVectorizable : std::integral_constant<bool,
                             std::is_same<T, int8>::value || std::is_same<T, uint8>::value ||
                             std::is_same<T, int16>::value || std::is_same<T, uint16>::value ||
                             std::is_same<T, int32>::value || std::is_same<T, uint32>::value ||
                             std::is_same<T, int64>::value || std::is_same<T, uint64>::value ||
                             std::is_same<T, float>::value || std::is_same<T, double>::value> {};

/// Number of T elements processed per vector operation for the active ISA.
template <class T>
inline size_t simdWidth() noexcept {
    static_assert(isVectorizable<T>::value, "simd::simdWidth: type is not vectorizable");
    return vectorBytes() / sizeof(T);
}

// ---------------------------------------------------------------------------
// Dispatch.
// ---------------------------------------------------------------------------

/// Which execution path an algorithm should take.
enum class DispatchMode : uint8 {
    Scalar = 0,
    Simd   = 1,
};

/// Minimum element count before the SIMD path pays off.
constexpr size_t simdLaneThreshold = 16;

/// Resolve an ExecutionPolicy into a runnable dispatch tier. Parallel tiers are
/// resolved later by the threading layer; this only decides scalar vs SIMD.
inline DispatchMode dispatchMode(const ExecutionPolicy& policy, size_t elements) noexcept {
    if (!simdAvailable()) return DispatchMode::Scalar;
    if (elements == 0) return DispatchMode::Scalar;
    if (policy.mode == ExecutionMode::Serial) return DispatchMode::Scalar;
    if (policy.mode == ExecutionMode::Simd || policy.mode == ExecutionMode::SimdParallel) {
        return DispatchMode::Simd;
    }
    // Auto / Parallel: prefer SIMD once the workload is large enough that a
    // full vector (of any width) can be exercised.
    if (elements >= simdLaneThreshold) return DispatchMode::Simd;
    return DispatchMode::Scalar;
}

// ---------------------------------------------------------------------------
// Work-load splitting.
// ---------------------------------------------------------------------------

/// Split a contiguous element range into [head, vector blocks, tail] so a
/// kernel can process whole vectors and finish scalars. Head consumes the
/// leading elements needed to reach a `byteStep`-aligned address; if the base
/// is not used the caller can pass a null pointer to get head == 0.
struct SimdSplit {
    size_t headBlocks   = 0; // scalar elements before the vector section
    size_t vectorElems  = 0; // total elements covered by full vectors
    size_t tail         = 0; // scalar elements after the vector section
};

template <class T>
inline SimdSplit split(size_t count, const void* base = nullptr) noexcept {
    const size_t bytes = vectorBytes();
    const size_t lanes = bytes / sizeof(T);
    if (lanes == 0 || count < lanes) return SimdSplit{count, 0, 0};
    size_t head = 0;
    if (base != nullptr) {
        const uintptr_t addr = reinterpret_cast<uintptr_t>(base);
        const uintptr_t mis  = addr & (bytes - 1);
        if (mis != 0) {
            const size_t padBytes = bytes - mis;
            head = padBytes / sizeof(T);
            if (head > count) head = count;
        }
    }
    const size_t remaining = count - head;
    const size_t blocks    = remaining / lanes;
    return SimdSplit{head, blocks * lanes, remaining - blocks * lanes};
}

// ---------------------------------------------------------------------------
// Reference kernels.
//
// These prove the vector path and are used by the self-check. Each kernel
// exists in a SIMD and a scalar form; callers select via dispatchMode(). The
// SIMD forms use unaligned loads so any properly aligned buffer simply
// performs better instead of requiring alignment.
// ---------------------------------------------------------------------------

/// dst[i] = (a[i] + b[i] + 1) >> 1 (rounding average).
IML_SIMD_ALWAYS_INLINE void averageU8Scalar(uint8* IML_SIMD_RESTRICT dst,
                                            const uint8* IML_SIMD_RESTRICT a,
                                            const uint8* IML_SIMD_RESTRICT b,
                                            size_t n) noexcept {
    for (size_t i = 0; i < n; ++i) {
        dst[i] = static_cast<uint8>((a[i] + b[i] + 1u) >> 1);
    }
}

/// dst[i] = min(a[i] + b[i], 255) (saturating add).
IML_SIMD_ALWAYS_INLINE void saturatingAddU8Scalar(uint8* IML_SIMD_RESTRICT dst,
                                                  const uint8* IML_SIMD_RESTRICT a,
                                                  const uint8* IML_SIMD_RESTRICT b,
                                                  size_t n) noexcept {
    for (size_t i = 0; i < n; ++i) {
        const unsigned v = static_cast<unsigned>(a[i]) + b[i];
        dst[i] = v > 255u ? 255u : static_cast<uint8>(v);
    }
}

#if IML_SIMD_SSE2
IML_SIMD_ALWAYS_INLINE void averageU8Sse2(uint8* IML_SIMD_RESTRICT dst,
                                          const uint8* IML_SIMD_RESTRICT a,
                                          const uint8* IML_SIMD_RESTRICT b,
                                          size_t n) noexcept {
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
        const __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
        const __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + i));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i), _mm_avg_epu8(va, vb));
    }
    averageU8Scalar(dst + i, a + i, b + i, n - i);
}

IML_SIMD_ALWAYS_INLINE void saturatingAddU8Sse2(uint8* IML_SIMD_RESTRICT dst,
                                                const uint8* IML_SIMD_RESTRICT a,
                                                const uint8* IML_SIMD_RESTRICT b,
                                                size_t n) noexcept {
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
        const __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
        const __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + i));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i), _mm_adds_epu8(va, vb));
    }
    saturatingAddU8Scalar(dst + i, a + i, b + i, n - i);
}
#endif // IML_SIMD_SSE2

#if IML_SIMD_NEON
IML_SIMD_ALWAYS_INLINE void averageU8Neon(uint8* IML_SIMD_RESTRICT dst,
                                          const uint8* IML_SIMD_RESTRICT a,
                                          const uint8* IML_SIMD_RESTRICT b,
                                          size_t n) noexcept {
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
        const uint8x16_t va = vld1q_u8(a + i);
        const uint8x16_t vb = vld1q_u8(b + i);
        vst1q_u8(dst + i, vrhaddq_u8(va, vb));
    }
    averageU8Scalar(dst + i, a + i, b + i, n - i);
}

IML_SIMD_ALWAYS_INLINE void saturatingAddU8Neon(uint8* IML_SIMD_RESTRICT dst,
                                                const uint8* IML_SIMD_RESTRICT a,
                                                const uint8* IML_SIMD_RESTRICT b,
                                                size_t n) noexcept {
    size_t i = 0;
    for (; i + 16 <= n; i += 16) {
        const uint8x16_t va = vld1q_u8(a + i);
        const uint8x16_t vb = vld1q_u8(b + i);
        vst1q_u8(dst + i, vqaddq_u8(va, vb));
    }
    saturatingAddU8Scalar(dst + i, a + i, b + i, n - i);
}
#endif // IML_SIMD_NEON

inline void averageU8(uint8* dst, const uint8* a, const uint8* b, size_t n) noexcept {
#if IML_SIMD_NEON
    averageU8Neon(dst, a, b, n);
#elif IML_SIMD_SSE2
    averageU8Sse2(dst, a, b, n);
#else
    averageU8Scalar(dst, a, b, n);
#endif
}

inline void saturatingAddU8(uint8* dst, const uint8* a, const uint8* b, size_t n) noexcept {
#if IML_SIMD_NEON
    saturatingAddU8Neon(dst, a, b, n);
#elif IML_SIMD_SSE2
    saturatingAddU8Sse2(dst, a, b, n);
#else
    saturatingAddU8Scalar(dst, a, b, n);
#endif
}

} // namespace simd
} // namespace iml