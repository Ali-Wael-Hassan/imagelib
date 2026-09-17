#pragma once
/// @file Simd.h
/// SIMD and execution dispatch primitives for image kernels: ISA feature
/// detection, fixed-size vector types, and work-load splitting.

#include "imagelib/core/Types.h"
#include "imagelib/core/ExecutionPolicy.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#if defined(__GNUC__) || defined(__clang__)
#define IML_SIMD_ALWAYS_INLINE inline __attribute__((always_inline))
#define IML_SIMD_RESTRICT __restrict__
#else
#define IML_SIMD_ALWAYS_INLINE inline
#define IML_SIMD_RESTRICT
#endif

#if (defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__)) && !defined(__SSE2__)
#define IML_SIMD_SSE2_FORCED
#else
#define IML_SIMD_SSE2_FORCED 0
#endif

#if defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__)
#define IML_SIMD_SSE2 1
#else
#define IML_SIMD_SSE2 0
#endif

#if defined(__SSE4_1__)
#define IML_SIMD_SSE41 1
#else
#define IML_SIMD_SSE41 0
#endif

#if defined(__AVX2__)
#define IML_SIMD_AVX2 1
#else
#define IML_SIMD_AVX2 0
#endif

#if defined(__AVX512F__)
#define IML_SIMD_AVX512F 1
#else
#define IML_SIMD_AVX512F 0
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
#define IML_SIMD_NEON 1
#else
#define IML_SIMD_NEON 0
#endif

#if IML_SIMD_SSE2
#include <emmintrin.h>
#endif
#if IML_SIMD_SSE41
#include <smmintrin.h>
#endif
#if IML_SIMD_AVX2
#include <immintrin.h>
#endif
#if IML_SIMD_NEON
#include <arm_neon.h>
#endif

namespace iml {
namespace simd {

/// Bit-flag SIMD ISA features. The auditable mask is a plain uint64 so it can
/// be inspected, serialized and combined without extra machinery.
enum SimdFeature : uint64 {
    Sse2 = 1ull << 0,
    Sse41 = 1ull << 1,
    Avx = 1ull << 2,
    Avx2 = 1ull << 3,
    Avx512F = 1ull << 4,
    Neon = 1ull << 5,
    SVE = 1ull << 6,
};

/// Set of SIMD features represented as a bitmask.
struct SimdFeatures {
    uint64 mask = 0;

    /// Default-constructs an empty feature set.
    constexpr SimdFeatures() noexcept = default;
    /// Constructs from a raw bitmask.
    constexpr explicit SimdFeatures(uint64 m) noexcept : mask(m) {}
    /// Constructs a single-feature set.
    constexpr explicit SimdFeatures(SimdFeature f) noexcept : mask(static_cast<uint64>(f)) {}

    /// True when feature `f` is present.
    constexpr bool has(SimdFeature f) const noexcept {
        return (mask & static_cast<uint64>(f)) != 0;
    }
    /// True when no features are present.
    constexpr bool empty() const noexcept { return mask == 0; }

    constexpr SimdFeatures operator|(const SimdFeatures& o) const noexcept {
        return SimdFeatures(mask | o.mask);
    }
    constexpr SimdFeatures operator&(const SimdFeatures& o) const noexcept {
        return SimdFeatures(mask & o.mask);
    }
    SimdFeatures& operator|=(const SimdFeatures& o) noexcept {
        mask |= o.mask;
        return *this;
    }
    SimdFeatures& operator&=(const SimdFeatures& o) noexcept {
        mask &= o.mask;
        return *this;
    }

    constexpr bool operator==(const SimdFeatures& o) const noexcept { return mask == o.mask; }
    constexpr bool operator!=(const SimdFeatures& o) const noexcept { return mask != o.mask; }
};

/// ISA features the compiler was built for (intrinsics available without
/// runtime guarding).
constexpr SimdFeatures compileTimeFeatures() noexcept {
    uint64 m = 0;
    if (IML_SIMD_SSE2)
        m |= uint64(Sse2);
    if (IML_SIMD_SSE41)
        m |= uint64(Sse41);
    if (IML_SIMD_AVX2)
        m |= uint64(Avx2);
    if (IML_SIMD_AVX512F)
        m |= uint64(Avx512F);
    if (IML_SIMD_NEON)
        m |= uint64(Neon);
    return SimdFeatures(m);
}

/// ISA features reported by the CPU at runtime (independent of what the
/// compiler can emit). Implemented in src/simd/Simd.cpp.
/// @return The supported ISA feature mask.
SimdFeatures cpuFeatures() noexcept;

/// Features we are safe to actually execute: compiled in AND supported by the
/// CPU. On x86-64 the SSE2 tier always survives; higher tiers are gated on the
/// detected hardware.
/// @return The executable feature mask.
SimdFeatures assuredFeatures() noexcept;

/// True when at least one vector ISA is available (128-bit tier on x86-64 /
/// AArch64, larger tiers when enabled and detected).
inline bool simdAvailable() noexcept {
    return assuredFeatures().has(Sse2) || assuredFeatures().has(Neon) ||
           assuredFeatures().has(Avx2) || assuredFeatures().has(Avx512F) ||
           assuredFeatures().has(SVE);
}

/// Bytes processed per vector operation. The 128-bit baseline is always
/// available on 64-bit x86/ARM targets; larger widths are only selected when
/// the ISA is both compiled in and detected at runtime.
/// @return Vector width in bytes, 0 when vectorization is unavailable.
size_t vectorBytes() noexcept;

/// Short human-readable description of the active ISA ("SSE2", "AVX2", ...).
/// @return The ISA name, or "scalar" when vectorization is unavailable.
const char* activeIsaName() noexcept;

/// True when T is a supported vectorizable element type.
template <class T>
struct isVectorizable : std::integral_constant<
                            bool,
                            std::is_same<T, int8>::value || std::is_same<T, uint8>::value ||
                                std::is_same<T, int16>::value || std::is_same<T, uint16>::value ||
                                std::is_same<T, int32>::value || std::is_same<T, uint32>::value ||
                                std::is_same<T, int64>::value || std::is_same<T, uint64>::value ||
                                std::is_same<T, float>::value || std::is_same<T, double>::value> {};

/// Number of T elements processed per vector operation for the active ISA.
/// @tparam T Vectorizable element type.
/// @return Element count per vector.
template <class T> inline size_t simdWidth() noexcept {
    static_assert(isVectorizable<T>::value, "simd::simdWidth: type is not vectorizable");
    return vectorBytes() / sizeof(T);
}

/// Which execution path an algorithm should take.
enum class DispatchMode : uint8 {
    Scalar = 0,
    Simd = 1,
};

/// Minimum element count before the SIMD path pays off.
constexpr size_t simdLaneThreshold = 16;

/// Resolve an ExecutionPolicy into a runnable dispatch tier. Parallel tiers are
/// resolved later by the threading layer; this only decides scalar vs SIMD.
/// @param elements Number of elements to process.
/// @return The dispatch mode an algorithm should use for this workload.
inline DispatchMode dispatchMode(const ExecutionPolicy& policy, size_t elements) noexcept {
    if (!simdAvailable())
        return DispatchMode::Scalar;
    if (elements == 0)
        return DispatchMode::Scalar;
    if (policy.mode == ExecutionMode::Serial)
        return DispatchMode::Scalar;
    if (policy.mode == ExecutionMode::Simd || policy.mode == ExecutionMode::SimdParallel) {
        return DispatchMode::Simd;
    }
    if (elements >= simdLaneThreshold)
        return DispatchMode::Simd;
    return DispatchMode::Scalar;
}

/// Split of a contiguous element range into head, vector blocks, and tail.
struct SimdSplit {
    /// Scalar elements before the vector section.
    size_t headBlocks = 0;
    /// Total elements covered by full vectors.
    size_t vectorElems = 0;
    /// Scalar elements after the vector section.
    size_t tail = 0;
};

/// Split a contiguous range into an aligned head, full vectors, and a scalar
/// tail for the active ISA.
/// @tparam T Vectorizable element type.
/// @param count Number of elements in the range.
/// @param base Optional base address used to align the vector section (null
///             when alignment is not required, giving head == 0).
/// @return The range split.
template <class T> inline SimdSplit split(size_t count, const void* base = nullptr) noexcept {
    const size_t bytes = vectorBytes();
    const size_t lanes = bytes / sizeof(T);
    if (lanes == 0 || count < lanes)
        return SimdSplit{count, 0, 0};
    size_t head = 0;
    if (base != nullptr) {
        const uintptr_t addr = reinterpret_cast<uintptr_t>(base);
        const uintptr_t mis = addr & (bytes - 1);
        if (mis != 0) {
            const size_t padBytes = bytes - mis;
            head = padBytes / sizeof(T);
            if (head > count)
                head = count;
        }
    }
    const size_t remaining = count - head;
    const size_t blocks = remaining / lanes;
    return SimdSplit{head, blocks * lanes, remaining - blocks * lanes};
}

/// Fixed-size vector of N lanes of T. Kernels use these instead of raw
/// intrinsics; data is read/written via load()/store() (unaligned by default).
/// @tparam T Element type (must be vectorizable).
/// @tparam N Lane count in [1, 4].
template <class T, int N> struct SimdVec {
    static_assert(N >= 1 && N <= 4, "simd::SimdVec supports N in [1,4]");
    static_assert(isVectorizable<T>::value, "simd::SimdVec: element type is not vectorizable");

    using value_type = T;
    static constexpr int Lanes = N;
    /// Size of this vector in bytes.
    static constexpr size_t bytes() noexcept { return sizeof(T) * static_cast<size_t>(N); }

    T v[N];

    /// Default-constructs a zero vector.
    constexpr SimdVec() noexcept : v{} {}
    /// Constructs from N element values (one per lane).
    /// @tparam Args Must be exactly N argument types.
    template <class... Args, std::enable_if_t<sizeof...(Args) == N, int> = 0>
    constexpr SimdVec(Args... args) noexcept : v{static_cast<T>(args)...} {}

    constexpr T& operator[](int i) noexcept { return v[i]; }
    constexpr const T& operator[](int i) const noexcept { return v[i]; }

    /// First lane component.
    constexpr T& x() noexcept { return v[0]; }
    /// First lane component (read-only).
    constexpr const T& x() const noexcept { return v[0]; }
    /// Second lane component; requires N >= 2.
    constexpr T& y() noexcept {
        static_assert(N >= 2, "simd::SimdVec: no y component");
        return v[1];
    }
    /// Second lane component (read-only); requires N >= 2.
    constexpr const T& y() const noexcept {
        static_assert(N >= 2, "simd::SimdVec: no y component");
        return v[1];
    }
    /// Third lane component; requires N >= 3.
    constexpr T& z() noexcept {
        static_assert(N >= 3, "simd::SimdVec: no z component");
        return v[2];
    }
    /// Third lane component (read-only); requires N >= 3.
    constexpr const T& z() const noexcept {
        static_assert(N >= 3, "simd::SimdVec: no z component");
        return v[2];
    }
    /// Fourth lane component; requires N >= 4.
    constexpr T& w() noexcept {
        static_assert(N >= 4, "simd::SimdVec: no w component");
        return v[3];
    }
    /// Fourth lane component (read-only); requires N >= 4.
    constexpr const T& w() const noexcept {
        static_assert(N >= 4, "simd::SimdVec: no w component");
        return v[3];
    }

    IML_SIMD_ALWAYS_INLINE SimdVec operator+(const SimdVec& o) const noexcept {
        SimdVec r;
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(r.v, _mm_add_ps(_mm_loadu_ps(v), _mm_loadu_ps(o.v)));
            return r;
        }
#endif
        for (int i = 0; i < N; ++i)
            r.v[i] = static_cast<T>(v[i] + o.v[i]);
        return r;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec operator-(const SimdVec& o) const noexcept {
        SimdVec r;
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(r.v, _mm_sub_ps(_mm_loadu_ps(v), _mm_loadu_ps(o.v)));
            return r;
        }
#endif
        for (int i = 0; i < N; ++i)
            r.v[i] = static_cast<T>(v[i] - o.v[i]);
        return r;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec operator*(const SimdVec& o) const noexcept {
        SimdVec r;
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(r.v, _mm_mul_ps(_mm_loadu_ps(v), _mm_loadu_ps(o.v)));
            return r;
        }
#endif
        for (int i = 0; i < N; ++i)
            r.v[i] = static_cast<T>(v[i] * o.v[i]);
        return r;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec operator/(const SimdVec& o) const noexcept {
        SimdVec r;
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(r.v, _mm_div_ps(_mm_loadu_ps(v), _mm_loadu_ps(o.v)));
            return r;
        }
#endif
        for (int i = 0; i < N; ++i)
            r.v[i] = static_cast<T>(v[i] / o.v[i]);
        return r;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec operator+(T s) const noexcept {
        SimdVec r;
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(r.v, _mm_add_ps(_mm_loadu_ps(v), _mm_set1_ps(s)));
            return r;
        }
#endif
        for (int i = 0; i < N; ++i)
            r.v[i] = static_cast<T>(v[i] + s);
        return r;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec operator-(T s) const noexcept {
        SimdVec r;
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(r.v, _mm_sub_ps(_mm_loadu_ps(v), _mm_set1_ps(s)));
            return r;
        }
#endif
        for (int i = 0; i < N; ++i)
            r.v[i] = static_cast<T>(v[i] - s);
        return r;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec operator*(T s) const noexcept {
        SimdVec r;
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(r.v, _mm_mul_ps(_mm_loadu_ps(v), _mm_set1_ps(s)));
            return r;
        }
#endif
        for (int i = 0; i < N; ++i)
            r.v[i] = static_cast<T>(v[i] * s);
        return r;
    }
    constexpr SimdVec operator/(T s) const noexcept {
        SimdVec r;
        for (int i = 0; i < N; ++i)
            r.v[i] = static_cast<T>(v[i] / s);
        return r;
    }
    friend constexpr SimdVec operator+(T s, const SimdVec& v) noexcept { return v + s; }
    friend constexpr SimdVec operator*(T s, const SimdVec& v) noexcept { return v * s; }

    IML_SIMD_ALWAYS_INLINE SimdVec operator-() const noexcept {
        SimdVec r;
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(r.v, _mm_sub_ps(_mm_setzero_ps(), _mm_loadu_ps(v)));
            return r;
        }
#endif
        for (int i = 0; i < N; ++i)
            r.v[i] = static_cast<T>(-v[i]);
        return r;
    }

    IML_SIMD_ALWAYS_INLINE SimdVec& operator+=(const SimdVec& o) noexcept {
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(v, _mm_add_ps(_mm_loadu_ps(v), _mm_loadu_ps(o.v)));
            return *this;
        }
#endif
        for (int i = 0; i < N; ++i)
            v[i] = static_cast<T>(v[i] + o.v[i]);
        return *this;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec& operator-=(const SimdVec& o) noexcept {
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(v, _mm_sub_ps(_mm_loadu_ps(v), _mm_loadu_ps(o.v)));
            return *this;
        }
#endif
        for (int i = 0; i < N; ++i)
            v[i] = static_cast<T>(v[i] - o.v[i]);
        return *this;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec& operator*=(const SimdVec& o) noexcept {
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(v, _mm_mul_ps(_mm_loadu_ps(v), _mm_loadu_ps(o.v)));
            return *this;
        }
#endif
        for (int i = 0; i < N; ++i)
            v[i] = static_cast<T>(v[i] * o.v[i]);
        return *this;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec& operator/=(const SimdVec& o) noexcept {
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(v, _mm_div_ps(_mm_loadu_ps(v), _mm_loadu_ps(o.v)));
            return *this;
        }
#endif
        for (int i = 0; i < N; ++i)
            v[i] = static_cast<T>(v[i] / o.v[i]);
        return *this;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec& operator+=(T s) noexcept {
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(v, _mm_add_ps(_mm_loadu_ps(v), _mm_set1_ps(s)));
            return *this;
        }
#endif
        for (int i = 0; i < N; ++i)
            v[i] = static_cast<T>(v[i] + s);
        return *this;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec& operator-=(T s) noexcept {
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(v, _mm_sub_ps(_mm_loadu_ps(v), _mm_set1_ps(s)));
            return *this;
        }
#endif
        for (int i = 0; i < N; ++i)
            v[i] = static_cast<T>(v[i] - s);
        return *this;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec& operator*=(T s) noexcept {
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(v, _mm_mul_ps(_mm_loadu_ps(v), _mm_set1_ps(s)));
            return *this;
        }
#endif
        for (int i = 0; i < N; ++i)
            v[i] = static_cast<T>(v[i] * s);
        return *this;
    }
    IML_SIMD_ALWAYS_INLINE SimdVec& operator/=(T s) noexcept {
#if IML_SIMD_SSE2
        if constexpr (std::is_same<T, float>::value && N == 4) {
            _mm_storeu_ps(v, _mm_div_ps(_mm_loadu_ps(v), _mm_set1_ps(s)));
            return *this;
        }
#endif
        for (int i = 0; i < N; ++i)
            v[i] = static_cast<T>(v[i] / s);
        return *this;
    }

    constexpr bool operator==(const SimdVec& o) const noexcept {
        for (int i = 0; i < N; ++i)
            if (!(v[i] == o.v[i]))
                return false;
        return true;
    }
    constexpr bool operator!=(const SimdVec& o) const noexcept { return !(*this == o); }
};

/// Alias for SimdVec<T, N>.
template <class T, int N> using Vec = SimdVec<T, N>;

/// Fixed-point convenience aliases.
using Float1 = SimdVec<float, 1>;
using Float2 = SimdVec<float, 2>;
using Float3 = SimdVec<float, 3>;
using Float4 = SimdVec<float, 4>;
using Int1 = SimdVec<int32, 1>;
using Int2 = SimdVec<int32, 2>;
using Int3 = SimdVec<int32, 3>;
using Int4 = SimdVec<int32, 4>;

static_assert(
    sizeof(Float1) == sizeof(float) * 1 && sizeof(Float2) == sizeof(float) * 2 &&
        sizeof(Float3) == sizeof(float) * 3 && sizeof(Float4) == sizeof(float) * 4 &&
        sizeof(Int1) == sizeof(int32) * 1 && sizeof(Int4) == sizeof(int32) * 4,
    "simd::SimdVec must be exactly N elements of T (no padding)");

/// Vector with all lanes loaded from a scalar.
/// @param s Value replicated across all lanes.
/// @return A vector whose lanes all equal `s`.
template <class T, int N> constexpr SimdVec<T, N> broadcast(T s) noexcept {
    SimdVec<T, N> r;
    for (int i = 0; i < N; ++i)
        r.v[i] = s;
    return r;
}

/// Unaligned load of N consecutive elements.
/// @param p Pointer to the first element.
/// @return A vector loaded from `p`.
template <class T, int N> IML_SIMD_ALWAYS_INLINE SimdVec<T, N> load(const T* p) noexcept {
    SimdVec<T, N> r;
#if IML_SIMD_SSE2
    if constexpr (std::is_same<T, float>::value && N == 4) {
        _mm_storeu_ps(r.v, _mm_loadu_ps(p));
        return r;
    }
#endif
    std::memcpy(r.v, p, sizeof(r.v));
    return r;
}

/// Unaligned store of N consecutive elements.
/// @param p Destination pointer.
/// @param v Vector to store.
template <class T, int N>
IML_SIMD_ALWAYS_INLINE void store(T* p, const SimdVec<T, N>& v) noexcept {
#if IML_SIMD_SSE2
    if constexpr (std::is_same<T, float>::value && N == 4) {
        _mm_storeu_ps(p, _mm_loadu_ps(v.v));
        return;
    }
#endif
    std::memcpy(p, v.v, sizeof(v.v));
}

/// Elementwise minimum.
/// @param a First operand.
/// @param b Second operand.
/// @return Vector with min(a[i], b[i]) per lane.
template <class T, int N>
IML_SIMD_ALWAYS_INLINE SimdVec<T, N> min(const SimdVec<T, N>& a, const SimdVec<T, N>& b) noexcept {
    SimdVec<T, N> r;
#if IML_SIMD_SSE2
    if constexpr (std::is_same<T, float>::value && N == 4) {
        _mm_storeu_ps(r.v, _mm_min_ps(_mm_loadu_ps(a.v), _mm_loadu_ps(b.v)));
        return r;
    }
#endif
    for (int i = 0; i < N; ++i)
        r.v[i] = a.v[i] < b.v[i] ? a.v[i] : b.v[i];
    return r;
}

/// Elementwise maximum.
/// @param a First operand.
/// @param b Second operand.
/// @return Vector with max(a[i], b[i]) per lane.
template <class T, int N>
IML_SIMD_ALWAYS_INLINE SimdVec<T, N> max(const SimdVec<T, N>& a, const SimdVec<T, N>& b) noexcept {
    SimdVec<T, N> r;
#if IML_SIMD_SSE2
    if constexpr (std::is_same<T, float>::value && N == 4) {
        _mm_storeu_ps(r.v, _mm_max_ps(_mm_loadu_ps(a.v), _mm_loadu_ps(b.v)));
        return r;
    }
#endif
    for (int i = 0; i < N; ++i)
        r.v[i] = a.v[i] > b.v[i] ? a.v[i] : b.v[i];
    return r;
}

/// Elementwise absolute value (int/float).
/// @param a Input vector.
/// @return Vector with |a[i]| per lane.
template <class T, int N>
IML_SIMD_ALWAYS_INLINE SimdVec<T, N> abs(const SimdVec<T, N>& a) noexcept {
    SimdVec<T, N> r;
#if IML_SIMD_SSE2
    if constexpr (std::is_same<T, float>::value && N == 4) {
        _mm_storeu_ps(r.v, _mm_andnot_ps(_mm_set1_ps(-0.0f), _mm_loadu_ps(a.v)));
        return r;
    }
#endif
    for (int i = 0; i < N; ++i)
        r.v[i] = a.v[i] < T(0) ? static_cast<T>(-a.v[i]) : a.v[i];
    return r;
}

/// Elementwise clamp to [lo, hi].
/// @param a Input vector.
/// @param lo Lower bound.
/// @param hi Upper bound.
/// @return Vector with each lane clamped.
template <class T, int N>
IML_SIMD_ALWAYS_INLINE SimdVec<T, N> clamp(const SimdVec<T, N>& a, T lo, T hi) noexcept {
#if IML_SIMD_SSE2
    if constexpr (std::is_same<T, float>::value && N == 4) {
        SimdVec<T, N> r;
        const __m128 value = _mm_loadu_ps(a.v);
        _mm_storeu_ps(r.v, _mm_min_ps(_mm_max_ps(value, _mm_set1_ps(lo)), _mm_set1_ps(hi)));
        return r;
    }
#endif
    SimdVec<T, N> r;
    for (int i = 0; i < N; ++i)
        r.v[i] = a.v[i] < lo ? lo : (a.v[i] > hi ? hi : a.v[i]);
    return r;
}

/// Elementwise clamp to [0, 1] (floating point).
/// @param a Input vector.
/// @return Vector with each lane clamped to [0, 1].
template <int N>
IML_SIMD_ALWAYS_INLINE SimdVec<float, N> clamp01(const SimdVec<float, N>& a) noexcept {
    return clamp(a, 0.f, 1.f);
}

/// Horizontal sum of all lanes.
/// @param a Input vector.
/// @return The sum of all lanes.
template <class T, int N> constexpr T sum(const SimdVec<T, N>& a) noexcept {
    T s = T(0);
    for (int i = 0; i < N; ++i)
        s = static_cast<T>(s + a.v[i]);
    return s;
}

/// Dot product.
/// @param a First operand.
/// @param b Second operand.
/// @return The sum of a[i] * b[i] over all lanes.
template <class T, int N> constexpr T dot(const SimdVec<T, N>& a, const SimdVec<T, N>& b) noexcept {
    T s = T(0);
    for (int i = 0; i < N; ++i)
        s = static_cast<T>(s + a.v[i] * b.v[i]);
    return s;
}

/// Elementwise floor (floating point; expressive only, no horizontal math).
/// @param a Input vector.
/// @return Vector with floor applied per lane.
template <int N> constexpr SimdVec<float, N> floor(const SimdVec<float, N>& a) noexcept {
    SimdVec<float, N> r;
    for (int i = 0; i < N; ++i) {
        const float f = a.v[i];
        r.v[i] = std::floor(f);
    }
    return r;
}

/// dst[i] = (a[i] + b[i] + 1) >> 1 (rounding average).
IML_SIMD_ALWAYS_INLINE void averageU8Scalar(
    uint8* IML_SIMD_RESTRICT dst,
    const uint8* IML_SIMD_RESTRICT a,
    const uint8* IML_SIMD_RESTRICT b,
    size_t n) noexcept {
    for (size_t i = 0; i < n; ++i) {
        dst[i] = static_cast<uint8>((a[i] + b[i] + 1u) >> 1);
    }
}

/// dst[i] = min(a[i] + b[i], 255) (saturating add).
IML_SIMD_ALWAYS_INLINE void saturatingAddU8Scalar(
    uint8* IML_SIMD_RESTRICT dst,
    const uint8* IML_SIMD_RESTRICT a,
    const uint8* IML_SIMD_RESTRICT b,
    size_t n) noexcept {
    for (size_t i = 0; i < n; ++i) {
        const unsigned v = static_cast<unsigned>(a[i]) + b[i];
        dst[i] = v > 255u ? 255u : static_cast<uint8>(v);
    }
}

#if IML_SIMD_SSE2
/// SSE2 specialization of averageU8.
IML_SIMD_ALWAYS_INLINE void averageU8Sse2(
    uint8* IML_SIMD_RESTRICT dst,
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
/// SSE2 specialization of saturatingAddU8.
IML_SIMD_ALWAYS_INLINE void saturatingAddU8Sse2(
    uint8* IML_SIMD_RESTRICT dst,
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
#endif

#if IML_SIMD_NEON
/// NEON specialization of averageU8.
IML_SIMD_ALWAYS_INLINE void averageU8Neon(
    uint8* IML_SIMD_RESTRICT dst,
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
/// NEON specialization of saturatingAddU8.
IML_SIMD_ALWAYS_INLINE void saturatingAddU8Neon(
    uint8* IML_SIMD_RESTRICT dst,
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
#endif
/// dst[i] = (a[i] + b[i] + 1) >> 1 using the best available ISA.
/// @param dst Output buffer, n bytes.
/// @param a First input buffer, n bytes.
/// @param b Second input buffer, n bytes.
/// @param n Number of elements (bytes).
inline void averageU8(uint8* dst, const uint8* a, const uint8* b, size_t n) noexcept {
#if IML_SIMD_NEON
    averageU8Neon(dst, a, b, n);
#elif IML_SIMD_SSE2
    averageU8Sse2(dst, a, b, n);
#else
    averageU8Scalar(dst, a, b, n);
#endif
}
/// dst[i] = min(a[i] + b[i], 255) using the best available ISA.
/// @param dst Output buffer, n bytes.
/// @param a First input buffer, n bytes.
/// @param b Second input buffer, n bytes.
/// @param n Number of elements (bytes).
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