#pragma once
// imagelib/math/Random.h
//
// Deterministic pseudo-random generation (PCG32 engine) plus lightweight
// hashing (splitmix64 / FNV-1a) used by procedural generation and noise.

#include "imagelib/core/Types.h"
#include "imagelib/math/Vector.h"

#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <limits>

namespace iml {
namespace math {

// ---------------------------------------------------------------------------
// Hashing
// ---------------------------------------------------------------------------

/// splitmix64 finalizer: high-quality 64-bit mixing.
inline uint64 hashU64(uint64 x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

/// FNV-1a over arbitrary bytes.
inline uint64 hashBytes(const void* data, size_t len, uint64 seed = 0xcbf29ce484222325ULL) noexcept {
    const byte* p = static_cast<const byte*>(data);
    uint64 h = seed;
    for (size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

inline uint64 hashString(const char* s) noexcept {
    return hashBytes(s, std::char_traits<char>::length(s));
}

inline uint64 hashCombine(uint64 a, uint64 b) noexcept {
    return a ^ (hashU64(b) + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
}

// ---------------------------------------------------------------------------
// PCG32 engine
// ---------------------------------------------------------------------------

class Rng {
public:
    Rng() noexcept { seed(0x853c49e6748fea9bULL); }
    explicit Rng(uint64 initialSeed) noexcept { seed(initialSeed); }

    void seed(uint64 s) noexcept {
        state_ = 0;
        inc_   = (s << 1u) | 1u;
        nextU32(); // warm up
        state_ += s;
        nextU32();
    }

    uint32 nextU32() noexcept {
        uint64 old = state_;
        state_ = old * 6364136223846793005ULL + inc_;
        uint32 xorshifted = static_cast<uint32>(((old >> 18u) ^ old) >> 27u);
        uint32 rot = static_cast<uint32>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
    }

    uint64 nextU64() noexcept {
        return (static_cast<uint64>(nextU32()) << 32) | nextU32();
    }

    /// Uniform uint32 in [0, maxInclusive].
    uint32 nextInt(uint32 maxInclusive) noexcept {
        if (maxInclusive == 0) return 0;
        uint32 mask = maxInclusive;
        mask |= mask >> 1; mask |= mask >> 2; mask |= mask >> 4;
        mask |= mask >> 8; mask |= mask >> 16;
        uint32 v;
        do { v = nextU32() & mask; } while (v > maxInclusive);
        return v;
    }

    /// Uniform uint32 in [lo, hi].
    int32 nextInt(int32 lo, int32 hi) noexcept {
        if (hi < lo) { int32 t = lo; lo = hi; hi = t; }
        uint64 span = static_cast<uint64>(hi) - static_cast<uint64>(lo) + 1;
        return static_cast<int32>(lo + static_cast<int64>(nextU64() % span));
    }

    float nextFloat01() noexcept {
        return (nextU32() >> 8) * (1.0f / 16777216.0f);
    }
    float nextFloat(float a, float b) noexcept {
        return a + (b - a) * nextFloat01();
    }
    double nextDouble01() noexcept {
        return (nextU64() >> 11) * (1.0 / 9007199254740992.0);
    }
    double nextDouble(double a, double b) noexcept {
        return a + (b - a) * nextDouble01();
    }

    /// Uniform in [0,1] per component.
    Vec2 nextVec2() noexcept { return Vec2(nextFloat01(), nextFloat01()); }
    Vec3 nextVec3() noexcept { return Vec3(nextFloat01(), nextFloat01(), nextFloat01()); }
    Vec4 nextVec4() noexcept { return Vec4(nextFloat01(), nextFloat01(), nextFloat01(), nextFloat01()); }

    /// Unit vector uniformly distributed on the unit circle.
    Vec2 nextUnitCircle() noexcept {
        float a = nextFloat(0.0f, 6.283185307179586f);
        return Vec2(std::cos(a), std::sin(a));
    }
    /// Unit vector uniformly distributed on the unit sphere.
    Vec3 nextUnitSphere() noexcept {
        float u = nextFloat(-1.0f, 1.0f);
        float phi = nextFloat(0.0f, 6.283185307179586f);
        float r = std::sqrt(maxVal(0.0f, 1.0f - u * u));
        return Vec3(r * std::cos(phi), r * std::sin(phi), u);
    }

    /// Approximate standard normal via Box-Muller.
    double nextGaussian() noexcept {
        double u1 = maxVal(nextDouble01(), 1e-12);
        double u2 = nextDouble01();
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(6.283185307179586 * u2);
    }

private:
    uint64 state_ = 0;
    uint64 inc_   = 0;
};

} // namespace math
} // namespace iml