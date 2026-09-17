#pragma once
/// @file
/// Deterministic pseudo-random generation (PCG32 engine) plus lightweight
/// hashing (splitmix64 / FNV-1a).

#include "imagelib/core/Types.h"
#include "imagelib/math/Vector.h"

#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <limits>

namespace iml {
namespace math {

/// splitmix64 finalizer: high-quality 64-bit mixing.
/// @param x Input value.
/// @return Mixed 64-bit value.
inline uint64 hashU64(uint64 x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

/// FNV-1a over arbitrary bytes.
/// @param data Byte buffer.
/// @param len Buffer length in bytes.
/// @param seed Optional seed (defaults to the FNV offset basis).
/// @return Hash value.
inline uint64
hashBytes(const void* data, size_t len, uint64 seed = 0xcbf29ce484222325ULL) noexcept {
    const byte* p = static_cast<const byte*>(data);
    uint64 h = seed;
    for (size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

/// FNV-1a hash of a null-terminated string.
/// @param s Null-terminated string.
/// @return Hash value.
inline uint64 hashString(const char* s) noexcept {
    return hashBytes(s, std::char_traits<char>::length(s));
}

/// Combines two hash values.
/// @param a First hash.
/// @param b Second hash.
/// @return Combined hash.
inline uint64 hashCombine(uint64 a, uint64 b) noexcept {
    return a ^ (hashU64(b) + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
}

/// Deterministic pseudo-random engine (PCG32).
class Rng {
  public:
    /// Constructs with a fixed default seed.
    Rng() noexcept { seed(0x853c49e6748fea9bULL); }
    /// Constructs with the given seed.
    /// @param initialSeed Seed value.
    explicit Rng(uint64 initialSeed) noexcept { seed(initialSeed); }

    /// Re-seeds the engine.
    /// @param s Seed value.
    void seed(uint64 s) noexcept {
        state_ = 0;
        inc_ = (s << 1u) | 1u;
        nextU32();
        state_ += s;
        nextU32();
    }

    /// Returns a pseudo-random 32-bit value.
    /// @return Value in [0, 2^32).
    uint32 nextU32() noexcept {
        uint64 old = state_;
        state_ = old * 6364136223846793005ULL + inc_;
        uint32 xorshifted = static_cast<uint32>(((old >> 18u) ^ old) >> 27u);
        uint32 rot = static_cast<uint32>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
    }

    /// Returns a pseudo-random 64-bit value.
    /// @return Value in [0, 2^64).
    uint64 nextU64() noexcept { return (static_cast<uint64>(nextU32()) << 32) | nextU32(); }

    /// Uniform uint32 in [0, maxInclusive].
    /// @param maxInclusive Inclusive upper bound.
    /// @return Value in [0, maxInclusive].
    uint32 nextInt(uint32 maxInclusive) noexcept {
        if (maxInclusive == 0)
            return 0;
        uint32 mask = maxInclusive;
        mask |= mask >> 1;
        mask |= mask >> 2;
        mask |= mask >> 4;
        mask |= mask >> 8;
        mask |= mask >> 16;
        uint32 v;
        do {
            v = nextU32() & mask;
        } while (v > maxInclusive);
        return v;
    }

    /// Uniform int32 in [lo, hi].
    /// @param lo Lower bound.
    /// @param hi Upper bound.
    /// @return Value in [lo, hi].
    int32 nextInt(int32 lo, int32 hi) noexcept {
        if (hi < lo) {
            int32 t = lo;
            lo = hi;
            hi = t;
        }
        uint64 span = static_cast<uint64>(hi) - static_cast<uint64>(lo) + 1;
        return static_cast<int32>(lo + static_cast<int64>(nextU64() % span));
    }

    /// Uniform float in [0, 1].
    /// @return Value in [0, 1].
    float nextFloat01() noexcept { return (nextU32() >> 8) * (1.0f / 16777216.0f); }
    /// Uniform float in [a, b].
    /// @param a Lower bound.
    /// @param b Upper bound.
    /// @return Value in [a, b].
    float nextFloat(float a, float b) noexcept { return a + (b - a) * nextFloat01(); }
    /// Uniform double in [0, 1].
    /// @return Value in [0, 1].
    double nextDouble01() noexcept { return (nextU64() >> 11) * (1.0 / 9007199254740992.0); }
    /// Uniform double in [a, b].
    /// @param a Lower bound.
    /// @param b Upper bound.
    /// @return Value in [a, b].
    double nextDouble(double a, double b) noexcept { return a + (b - a) * nextDouble01(); }

    /// Vec2 with components uniform in [0, 1].
    /// @return Random vector in the unit square.
    Vec2 nextVec2() noexcept { return Vec2(nextFloat01(), nextFloat01()); }
    /// Vec3 with components uniform in [0, 1].
    /// @return Random vector in the unit cube.
    Vec3 nextVec3() noexcept { return Vec3(nextFloat01(), nextFloat01(), nextFloat01()); }
    /// Vec4 with components uniform in [0, 1].
    /// @return Random vector with components in [0, 1].
    Vec4 nextVec4() noexcept {
        return Vec4(nextFloat01(), nextFloat01(), nextFloat01(), nextFloat01());
    }

    /// Unit vector uniformly distributed on the unit circle.
    /// @return Random unit vector.
    Vec2 nextUnitCircle() noexcept {
        float a = nextFloat(0.0f, 6.283185307179586f);
        return Vec2(std::cos(a), std::sin(a));
    }
    /// Unit vector uniformly distributed on the unit sphere.
    /// @return Random unit vector.
    Vec3 nextUnitSphere() noexcept {
        float u = nextFloat(-1.0f, 1.0f);
        float phi = nextFloat(0.0f, 6.283185307179586f);
        float r = std::sqrt(maxVal(0.0f, 1.0f - u * u));
        return Vec3(r * std::cos(phi), r * std::sin(phi), u);
    }

    /// Approximate standard normal via Box-Muller.
    /// @return Sample with mean 0 and variance 1.
    double nextGaussian() noexcept {
        double u1 = maxVal(nextDouble01(), 1e-12);
        double u2 = nextDouble01();
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(6.283185307179586 * u2);
    }

  private:
    /// PCG internal state.
    uint64 state_ = 0;
    /// PCG increment.
    uint64 inc_ = 0;
};

} // namespace math
} // namespace iml