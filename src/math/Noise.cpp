// imagelib/src/math/Noise.cpp
//
// Out-of-line non-template noise bodies (see imagelib/math/Noise.h). All
// exposed 1D/2D/3D/4D noise returns values normalized to [0, 1] (mean ~0.5);
// fractal combinators return their natural ranges, which the Procedural
// layer re-normalizes where needed.

#include "imagelib/math/Noise.h"

#include <algorithm>
#include <cmath>

namespace iml {
namespace math {
namespace noise {

namespace {
inline float c01(float v) noexcept {
    return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
}

inline float remap01(float v) noexcept {
    return c01(0.5f + 0.5f * v); // [-1,1] -> [0,1]
}

inline bool odd(uint64 h) noexcept {
    return (h & 1u) != 0;
}
} // namespace

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

uint64 latticeHash(uint64 seed, int64 a, int64 b, int64 c, int64 d) noexcept {
    uint64 h = seed;
    h = hashCombine(h, static_cast<uint64>(a));
    h = hashCombine(h, static_cast<uint64>(b));
    h = hashCombine(h, static_cast<uint64>(c));
    h = hashCombine(h, static_cast<uint64>(d));
    return h;
}

uint64 latticeHash(uint64 seed, int64 a, int64 b, int64 c, int64 d,
                   int64 e, int64 f) noexcept {
    uint64 h = seed;
    h = hashCombine(h, static_cast<uint64>(a));
    h = hashCombine(h, static_cast<uint64>(b));
    h = hashCombine(h, static_cast<uint64>(c));
    h = hashCombine(h, static_cast<uint64>(d));
    h = hashCombine(h, static_cast<uint64>(e));
    h = hashCombine(h, static_cast<uint64>(f));
    return h;
}

float unit(uint64 h) noexcept {
    return static_cast<float>(h >> 40) * (1.0f / 16777216.0f);
}

float fade(float t) noexcept {
    return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
}

float fadeLerp(float a, float b, float t) noexcept {
    return a + (b - a) * fade(t);
}

float dotGrad2(uint64 h, float x, float y) noexcept {
    const float(&g)[2] = grad2Table[h & 7u];
    return g[0] * x + g[1] * y;
}

float dotGrad3(uint64 h, float x, float y, float z) noexcept {
    const float(&g)[3] = grad3Table[h & 15u];
    return g[0] * x + g[1] * y + g[2] * z;
}

// ---------------------------------------------------------------------------
// White noise (no interpolation)
// ---------------------------------------------------------------------------

float whiteNoise1D(uint64 seed, float x) noexcept {
    return unit(latticeHash(seed, static_cast<int64>(std::floor(x))));
}

float whiteNoise2D(uint64 seed, float x, float y) noexcept {
    return unit(latticeHash(seed,
                            static_cast<int64>(std::floor(x)),
                            static_cast<int64>(std::floor(y))));
}

float whiteNoise3D(uint64 seed, float x, float y, float z) noexcept {
    return unit(latticeHash(seed,
                            static_cast<int64>(std::floor(x)),
                            static_cast<int64>(std::floor(y)),
                            static_cast<int64>(std::floor(z))));
}

float whiteNoise4D(uint64 seed, float x, float y, float z, float w) noexcept {
    return unit(latticeHash(seed,
                            static_cast<int64>(std::floor(x)),
                            static_cast<int64>(std::floor(y)),
                            static_cast<int64>(std::floor(z)),
                            static_cast<int64>(std::floor(w))));
}

// ---------------------------------------------------------------------------
// Value noise (interpolated lattice hashes)
// ---------------------------------------------------------------------------

float valueNoise1D(uint64 seed, float x) noexcept {
    const int64 i = static_cast<int64>(std::floor(x));
    const float t = x - static_cast<float>(i);
    return fadeLerp(unit(latticeHash(seed, i)),
                    unit(latticeHash(seed, i + 1)), t);
}

float valueNoise2D(uint64 seed, float x, float y) noexcept {
    const int64 ix = static_cast<int64>(std::floor(x));
    const int64 iy = static_cast<int64>(std::floor(y));
    const float tx = x - static_cast<float>(ix);
    const float ty = y - static_cast<float>(iy);
    const float a = unit(latticeHash(seed, ix,     iy));
    const float b = unit(latticeHash(seed, ix + 1, iy));
    const float c = unit(latticeHash(seed, ix,     iy + 1));
    const float d = unit(latticeHash(seed, ix + 1, iy + 1));
    return fadeLerp(fadeLerp(a, b, tx), fadeLerp(c, d, tx), ty);
}

float valueNoise3D(uint64 seed, float x, float y, float z) noexcept {
    const int64 ix = static_cast<int64>(std::floor(x));
    const int64 iy = static_cast<int64>(std::floor(y));
    const int64 iz = static_cast<int64>(std::floor(z));
    const float tx = x - static_cast<float>(ix);
    const float ty = y - static_cast<float>(iy);
    const float tz = z - static_cast<float>(iz);
    auto corner = [&](int64 dx, int64 dy, int64 dz) {
        return unit(latticeHash(seed, ix + dx, iy + dy, iz + dz));
    };
    float x00 = fadeLerp(corner(0, 0, 0), corner(1, 0, 0), tx);
    float x10 = fadeLerp(corner(0, 1, 0), corner(1, 1, 0), tx);
    float x01 = fadeLerp(corner(0, 0, 1), corner(1, 0, 1), tx);
    float x11 = fadeLerp(corner(0, 1, 1), corner(1, 1, 1), tx);
    return fadeLerp(fadeLerp(x00, x10, ty), fadeLerp(x01, x11, ty), tz);
}

float valueNoise4D(uint64 seed, float x, float y, float z, float w) noexcept {
    const int64 ix = static_cast<int64>(std::floor(x));
    const int64 iy = static_cast<int64>(std::floor(y));
    const int64 iz = static_cast<int64>(std::floor(z));
    const int64 iw = static_cast<int64>(std::floor(w));
    const float tx = x - static_cast<float>(ix);
    const float ty = y - static_cast<float>(iy);
    const float tz = z - static_cast<float>(iz);
    const float tw = w - static_cast<float>(iw);
    auto corner = [&](int64 dx, int64 dy, int64 dz, int64 dw) {
        return unit(latticeHash(seed, ix + dx, iy + dy, iz + dz, iw + dw));
    };
    float r[2];
    for (int q = 0; q < 2; ++q) {
        const int64 dq = static_cast<int64>(q);
        float s00 = fadeLerp(corner(0, 0, 0, dq), corner(1, 0, 0, dq), tx);
        float s10 = fadeLerp(corner(0, 1, 0, dq), corner(1, 1, 0, dq), tx);
        float s01 = fadeLerp(corner(0, 0, 1, dq), corner(1, 0, 1, dq), tx);
        float s11 = fadeLerp(corner(0, 1, 1, dq), corner(1, 1, 1, dq), tx);
        r[q] = fadeLerp(fadeLerp(s00, s10, ty), fadeLerp(s01, s11, ty), tz);
    }
    return fadeLerp(r[0], r[1], tw);
}

// ---------------------------------------------------------------------------
// Gradient noise (Perlin-style gradient lattices) -> [0,1]
// ---------------------------------------------------------------------------

float gradientNoise1D(uint64 seed, float x) noexcept {
    const int64 i = static_cast<int64>(std::floor(x));
    const float t = x - static_cast<float>(i);
    const float g0 = odd(latticeHash(seed, i)) ? 1.f : -1.f;
    const float g1 = odd(latticeHash(seed, i + 1)) ? 1.f : -1.f;
    return remap01(g0 * t + (g1 * (t - 1.f) - g0 * t) * fade(t));
}

float gradientNoise2D(uint64 seed, float x, float y) noexcept {
    const int64 ix = static_cast<int64>(std::floor(x));
    const int64 iy = static_cast<int64>(std::floor(y));
    const float tx = x - static_cast<float>(ix);
    const float ty = y - static_cast<float>(iy);
    const float n00 = dotGrad2(latticeHash(seed, ix,     iy),     tx,     ty);
    const float n10 = dotGrad2(latticeHash(seed, ix + 1, iy),     tx - 1.f, ty);
    const float n01 = dotGrad2(latticeHash(seed, ix,     iy + 1), tx,     ty - 1.f);
    const float n11 = dotGrad2(latticeHash(seed, ix + 1, iy + 1), tx - 1.f, ty - 1.f);
    const float u = fade(tx);
    const float v = fade(ty);
    return remap01(1.3f * (n00 + (n10 - n00) * u
                          + ((n01 + (n11 - n01) * u) - (n00 + (n10 - n00) * u)) * v));
}

float gradientNoise3D(uint64 seed, float x, float y, float z) noexcept {
    const int64 ix = static_cast<int64>(std::floor(x));
    const int64 iy = static_cast<int64>(std::floor(y));
    const int64 iz = static_cast<int64>(std::floor(z));
    const float tx = x - static_cast<float>(ix);
    const float ty = y - static_cast<float>(iy);
    const float tz = z - static_cast<float>(iz);
    auto corner = [&](int64 dx, int64 dy, int64 dz) {
        return dotGrad3(latticeHash(seed, ix + dx, iy + dy, iz + dz),
                        tx - static_cast<float>(dx),
                        ty - static_cast<float>(dy),
                        tz - static_cast<float>(dz));
    };
    const float u = fade(tx), v = fade(ty), t = fade(tz);
    float n000 = corner(0, 0, 0), n100 = corner(1, 0, 0);
    float n010 = corner(0, 1, 0), n110 = corner(1, 1, 0);
    float n001 = corner(0, 0, 1), n101 = corner(1, 0, 1);
    float n011 = corner(0, 1, 1), n111 = corner(1, 1, 1);
    float nx00 = n000 + (n100 - n000) * u;
    float nx10 = n010 + (n110 - n010) * u;
    float nx01 = n001 + (n101 - n001) * u;
    float nx11 = n011 + (n111 - n011) * u;
    float ny0 = nx00 + (nx10 - nx00) * v;
    float ny1 = nx01 + (nx11 - nx01) * v;
    return remap01(0.28867513f * (ny0 + (ny1 - ny0) * t));
}

float gradientNoise4D(uint64 seed, float x, float y, float z, float w) noexcept {
    // Volume-average style: interpolate 2D slices in the w direction.
    return c01(valueNoise4D(seed, x, y, z, w));
}

// ---------------------------------------------------------------------------
// Alias: perlin == classic gradient noise lattice.
// ---------------------------------------------------------------------------

float perlinNoise1D(uint64 seed, float x) noexcept { return gradientNoise1D(seed, x); }
float perlinNoise2D(uint64 seed, float x, float y) noexcept { return gradientNoise2D(seed, x, y); }
float perlinNoise3D(uint64 seed, float x, float y, float z) noexcept { return gradientNoise3D(seed, x, y, z); }
float perlinNoise4D(uint64 seed, float x, float y, float z, float w) noexcept { return gradientNoise4D(seed, x, y, z, w); }

// ---------------------------------------------------------------------------
// Simplex noise (2D, 3D)
// ---------------------------------------------------------------------------

float simplexNoise2D(uint64 seed, float xin, float yin) noexcept {
    constexpr float F2 = 0.3660254037844386f;  // (sqrt(3) - 1) / 2
    constexpr float G2 = 0.21132486540518713f; // (3 - sqrt(3)) / 6
    const float s = (xin + yin) * F2;
    const float i = std::floor(xin + s);
    const float j = std::floor(yin + s);
    const float t = (i + j) * G2;
    const float x0 = xin - (i - t);
    const float y0 = yin - (j - t);
    const int i1 = x0 > y0 ? 1 : 0;
    const int j1 = x0 > y0 ? 0 : 1;
    const float x1 = x0 - static_cast<float>(i1) + G2;
    const float y1 = y0 - static_cast<float>(j1) + G2;
    const float x2 = x0 - 1.f + 2.f * G2;
    const float y2 = y0 - 1.f + 2.f * G2;
    const int64 ii = static_cast<int64>(std::floor(i));
    const int64 jj = static_cast<int64>(std::floor(j));
    float n0 = 0.f, n1 = 0.f, n2 = 0.f;
    float tt = 0.5f - x0 * x0 - y0 * y0;
    if (tt > 0.f) { tt *= tt; n0 = tt * tt * dotGrad2(latticeHash(seed, ii, jj), x0, y0); }
    tt = 0.5f - x1 * x1 - y1 * y1;
    if (tt > 0.f) { tt *= tt; n1 = tt * tt * dotGrad2(latticeHash(seed, ii + i1, jj + j1), x1, y1); }
    tt = 0.5f - x2 * x2 - y2 * y2;
    if (tt > 0.f) { tt *= tt; n2 = tt * tt * dotGrad2(latticeHash(seed, ii + 2, jj + 2), x2, y2); }
    return remap01(70.f * (n0 + n1 + n2));
}

float simplexNoise3D(uint64 seed, float xin, float yin, float zin) noexcept {
    constexpr float F3 = 1.f / 3.f;
    constexpr float G3 = 1.f / 6.f;
    const float s = (xin + yin + zin) * F3;
    const float i = std::floor(xin + s);
    const float j = std::floor(yin + s);
    const float k = std::floor(zin + s);
    const float t = (i + j + k) * G3;
    const float x0 = xin - (i - t);
    const float y0 = yin - (j - t);
    const float z0 = zin - (k - t);
    int i1, j1, k1, i2, j2, k2;
    if (x0 >= y0) {
        if (y0 >= z0)      { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
        else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; }
        else               { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; }
    } else {
        if (y0 < z0)       { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; }
        else if (x0 < z0)  { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; }
        else               { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
    }
    const float x1 = x0 - static_cast<float>(i1) + G3;
    const float y1 = y0 - static_cast<float>(j1) + G3;
    const float z1 = z0 - static_cast<float>(k1) + G3;
    const float x2 = x0 - static_cast<float>(i2) + 2.f * G3;
    const float y2 = y0 - static_cast<float>(j2) + 2.f * G3;
    const float z2 = z0 - static_cast<float>(k2) + 2.f * G3;
    const float x3 = x0 - 1.f + 3.f * G3;
    const float y3 = y0 - 1.f + 3.f * G3;
    const float z3 = z0 - 1.f + 3.f * G3;
    const int64 ii = static_cast<int64>(std::floor(i));
    const int64 jj = static_cast<int64>(std::floor(j));
    const int64 kk = static_cast<int64>(std::floor(k));
    float n0 = 0.f, n1 = 0.f, n2 = 0.f, n3 = 0.f;
    float tt = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if (tt > 0.f) { tt *= tt; n0 = tt * tt * dotGrad3(latticeHash(seed, ii, jj, kk, 0), x0, y0, z0); }
    tt = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if (tt > 0.f) { tt *= tt; n1 = tt * tt * dotGrad3(latticeHash(seed, ii + i1, jj + j1, kk + k1, 1), x1, y1, z1); }
    tt = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if (tt > 0.f) { tt *= tt; n2 = tt * tt * dotGrad3(latticeHash(seed, ii + i2, jj + j2, kk + k2, 2), x2, y2, z2); }
    tt = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if (tt > 0.f) { tt *= tt; n3 = tt * tt * dotGrad3(latticeHash(seed, ii + 1, jj + 1, kk + 1, 3), x3, y3, z3); }
    return remap01(32.f * (n0 + n1 + n2 + n3));
}

// ---------------------------------------------------------------------------
// Worley / cellular noise
// ---------------------------------------------------------------------------

float Worley2D::value() const noexcept { return c01(0.70710678f * f1); }   // /sqrt(2)
float Worley2D::edge() const noexcept { return c01(0.70710678f * (f2 - f1)); }
float Worley3D::value() const noexcept { return c01(0.57735027f * f1); }   // /sqrt(3)
float Worley3D::edge() const noexcept { return c01(0.57735027f * (f2 - f1)); }

Worley2D worley2D(uint64 seed, float x, float y) noexcept {
    const int64 xi = static_cast<int64>(std::floor(x));
    const int64 yi = static_cast<int64>(std::floor(y));
    float f1 = 1e30f, f2 = 1e30f;
    Vec2 closest(0.f, 0.f);
    const uint64 h0 = latticeHash(seed, xi, yi);
    for (int64 j = yi - 1; j <= yi + 1; ++j) {
        for (int64 i = xi - 1; i <= xi + 1; ++i) {
            const uint64 h = i == xi && j == yi ? h0 : latticeHash(seed, i, j);
            const float px = static_cast<float>(i) + unit(h);
            const float py = static_cast<float>(j) + unit(h >> 32);
            const float dx = x - px;
            const float dy = y - py;
            const float d2 = dx * dx + dy * dy;
            if (d2 < f2) {
                if (d2 < f1) {
                    f2 = f1;
                    f1 = d2;
                    closest = Vec2(dx, dy);
                } else {
                    f2 = d2;
                }
            }
        }
    }
    return Worley2D{std::sqrt(f1), std::sqrt(f2), closest};
}

Worley3D worley3D(uint64 seed, float x, float y, float z) noexcept {
    const int64 xi = static_cast<int64>(std::floor(x));
    const int64 yi = static_cast<int64>(std::floor(y));
    const int64 zi = static_cast<int64>(std::floor(z));
    float f1 = 1e30f, f2 = 1e30f;
    for (int64 k = zi - 1; k <= zi + 1; ++k) {
        for (int64 j = yi - 1; j <= yi + 1; ++j) {
            for (int64 i = xi - 1; i <= xi + 1; ++i) {
                const uint64 h = latticeHash(seed, i, j, k);
                const float px = static_cast<float>(i) + unit(h);
                const float py = static_cast<float>(j) + unit(h >> 32);
                const float pz = static_cast<float>(k) + unit(h >> 21);
                const float dx = x - px;
                const float dy = y - py;
                const float dz = z - pz;
                const float d2 = dx * dx + dy * dy + dz * dz;
                if (d2 < f2) {
                    if (d2 < f1) { f2 = f1; f1 = d2; }
                    else         { f2 = d2; }
                }
            }
        }
    }
    return Worley3D{std::sqrt(f1), std::sqrt(f2)};
}

// ---------------------------------------------------------------------------
// Fractal combinators
// ---------------------------------------------------------------------------

float fbm2D(uint64 seed, float x, float y, const FractalParams& p) noexcept {
    float amp = p.gain, freq = p.frequency, sum = 0.f, norm = 0.f;
    for (int o = 0; o < p.octaves; ++o) {
        sum += (2.f * gradientNoise2D(seed, x * freq, y * freq) - 1.f) * amp;
        norm += amp;
        freq *= p.lacunarity;
        amp  *= p.persistence;
    }
    return norm > 0.f ? sum / norm : 0.f;
}

float fbm3D(uint64 seed, float x, float y, float z, const FractalParams& p) noexcept {
    float amp = p.gain, freq = p.frequency, sum = 0.f, norm = 0.f;
    for (int o = 0; o < p.octaves; ++o) {
        sum += (2.f * gradientNoise3D(seed, x * freq, y * freq, z * freq) - 1.f) * amp;
        norm += amp;
        freq *= p.lacunarity;
        amp  *= p.persistence;
    }
    return norm > 0.f ? sum / norm : 0.f;
}

float ridged2D(uint64 seed, float x, float y, const FractalParams& p) noexcept {
    float amp = p.gain, freq = p.frequency, sum = 0.f, norm = 0.f;
    for (int o = 0; o < p.octaves; ++o) {
        const float base = 2.f * gradientNoise2D(seed, x * freq, y * freq) - 1.f;
        const float r = std::abs(1.f - std::abs(base));
        sum += r * r * amp;
        norm += amp;
        freq *= p.lacunarity;
        amp  *= p.persistence;
    }
    return norm > 0.f ? c01(sum / norm) : 0.f;
}

float billow2D(uint64 seed, float x, float y, const FractalParams& p) noexcept {
    float amp = p.gain, freq = p.frequency, sum = 0.f, norm = 0.f;
    for (int o = 0; o < p.octaves; ++o) {
        const float base = 2.f * gradientNoise2D(seed, x * freq, y * freq) - 1.f;
        sum += std::abs(base) * amp;
        norm += amp;
        freq *= p.lacunarity;
        amp  *= p.persistence;
    }
    return norm > 0.f ? c01(sum / norm) : 0.f;
}

float hybrid2D(uint64 seed, float x, float y, const FractalParams& p) noexcept {
    return 0.5f * fbm2D(seed, x, y, p) + 0.5f * (2.f * ridged2D(seed, x, y, p) - 1.f);
}

float domainWarp2D(uint64 seed, float x, float y, float amount,
                   const FractalParams& p) noexcept {
    const float ox = amount * fbm2D(seed, x, y, p);
    const float oy = amount * fbm2D(seed + 0x9e3779b9U, x, y, p);
    return fbm2D(seed + 0x85ebca6bU, x + ox, y + oy, p);
}

} // namespace noise
} // namespace math
} // namespace iml