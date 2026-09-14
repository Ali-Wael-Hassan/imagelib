#pragma once
#define IMAGELIB_MATH_NOISE_H_
// imagelib/math/Noise.h
//
// Generic mathematical noise: Value, Perlin ("improved", classic gradient),
// Simplex, Worley/Cellular, White, Gradient. Provides 1D/2D/3D/4D where
// practical, plus fractal combinators (FBM, Ridged, Billow, Hybrid, Domain
// warp). All output is normalized to [0, 1] unless stated.
//
// This is pure math; the Procedural subsystem converts it into images.
//
// constexpr tables and noise parameter structs live here; non-template noise
// bodies are compiled out-of-line in src/math/Noise.cpp; the generic fractal
// combinator template lives in Noise.tpp.

#include "imagelib/core/Types.h"
#include "imagelib/math/Scalar.h"
#include "imagelib/math/Random.h"

#include <cmath>

namespace iml {
namespace math {
namespace noise {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Deterministic, seed-dependent hash of up to four lattice coordinates.
uint64 latticeHash(uint64 seed, int64 a, int64 b = 0, int64 c = 0, int64 d = 0) noexcept;

uint64 latticeHash(uint64 seed, int64 a, int64 b, int64 d, int64 e, int64 f, int64 g) noexcept;

float unit(uint64 h) noexcept;

float fade(float t) noexcept;

/// Smoothly interpolates with the quintic fade.
float fadeLerp(float a, float b, float t) noexcept;

constexpr float grad2Table[8][2] = {
    { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 },
    { 1, 0 }, { -1, 0 }, { 0, 1 },  { 0, -1 },
};

constexpr float grad3Table[16][3] = {
    { 1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
    { 1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
    { 0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1},
    { 1,1,0}, {-1,1,0}, {0,-1,1}, {0,-1,-1},
};

float dotGrad2(uint64 h, float x, float y) noexcept;
float dotGrad3(uint64 h, float x, float y, float z) noexcept;

// ---------------------------------------------------------------------------
// White noise (no interpolation)
// ---------------------------------------------------------------------------

float whiteNoise1D(uint64 seed, float x) noexcept;
float whiteNoise2D(uint64 seed, float x, float y) noexcept;
float whiteNoise3D(uint64 seed, float x, float y, float z) noexcept;
float whiteNoise4D(uint64 seed, float x, float y, float z, float w) noexcept;

// ---------------------------------------------------------------------------
// Value noise (interpolated lattice hashes)
// ---------------------------------------------------------------------------

float valueNoise1D(uint64 seed, float x) noexcept;
float valueNoise2D(uint64 seed, float x, float y) noexcept;
float valueNoise3D(uint64 seed, float x, float y, float z) noexcept;
float valueNoise4D(uint64 seed, float x, float y, float z, float w) noexcept;

// ---------------------------------------------------------------------------
// Gradient noise (Perlin-style gradient lattices)
// ---------------------------------------------------------------------------

float gradientNoise1D(uint64 seed, float x) noexcept;
float gradientNoise2D(uint64 seed, float x, float y) noexcept;
float gradientNoise3D(uint64 seed, float x, float y, float z) noexcept;
float gradientNoise4D(uint64 seed, float x, float y, float z, float w) noexcept;

// ---------------------------------------------------------------------------
// Alias: perlin == classic gradient noise lattice.
// ---------------------------------------------------------------------------
float perlinNoise1D(uint64 seed, float x) noexcept;
float perlinNoise2D(uint64 seed, float x, float y) noexcept;
float perlinNoise3D(uint64 seed, float x, float y, float z) noexcept;
float perlinNoise4D(uint64 seed, float x, float y, float z, float w) noexcept;

// ---------------------------------------------------------------------------
// Simplex noise (2D, 3D)
// ---------------------------------------------------------------------------

float simplexNoise2D(uint64 seed, float xin, float yin) noexcept;
float simplexNoise3D(uint64 seed, float xin, float yin, float zin) noexcept;

// ---------------------------------------------------------------------------
// Worley / cellular noise
// ---------------------------------------------------------------------------

struct Worley2D {
    float f1; // distance to nearest feature point
    float f2; // distance to second nearest
    Vec2  closest; // F1 feature point offset within its cell
    float value() const noexcept;          // [0,1] scaled
    float edge() const noexcept;           // F2-F1 "crackle"
};

/// 2D cellular noise over the unit cell; returns distances, not colors.
Worley2D worley2D(uint64 seed, float x, float y) noexcept;

struct Worley3D {
    float f1;
    float f2;
    float value() const noexcept;
    float edge() const noexcept;
};

Worley3D worley3D(uint64 seed, float x, float y, float z) noexcept;

// ---------------------------------------------------------------------------
// Fractal combinators
// ---------------------------------------------------------------------------

struct FractalParams {
    int    octaves     = 4;
    float  frequency   = 1.0f;  // base lattice frequency
    float  lacunarity  = 2.0f;  // frequency multiplier per octave
    float  persistence = 0.5f;  // amplitude multiplier per octave
    float  gain        = 1.0f;  // amplitude of the first octave (value noise formula)
    float  ridgeOffset = 1.0f;  // buried under ridged noise bias
};

/// Fractional Brownian motion over a binary noise function.
/// `baseFn(seed, x, y)` returns base noise such that the FBM in [-1,1] space.
template <typename Fn>
float fbmGeneric(uint64 seed, float x, float y, const FractalParams& p, Fn baseFn) noexcept;

/// FBM built on gradient/perlin 2D (returns base noise ~[-1,1]).
float fbm2D(uint64 seed, float x, float y, const FractalParams& p = {}) noexcept;

float fbm3D(uint64 seed, float x, float y, float z, const FractalParams& p = {}) noexcept;

/// Ridged multifractal: octaves inverted on |noise| so ridges crest sharply.
float ridged2D(uint64 seed, float x, float y, const FractalParams& p = {}) noexcept;

/// Billow: abs() lobes still leave soft peaks (classic "pillow").
float billow2D(uint64 seed, float x, float y, const FractalParams& p = {}) noexcept;

/// Hybrid multifractal mixing fbm and ridged octaves.
float hybrid2D(uint64 seed, float x, float y, const FractalParams& p = {}) noexcept;

/// Domain warping: sample noise at coordinates displaced by two lower-octave
/// noise fields (classic "stretched/smudged" manifolds).
float domainWarp2D(uint64 seed, float x, float y,
                   float amount = 1.0f, const FractalParams& wp = {}) noexcept;

} // namespace noise
} // namespace math
} // namespace iml

#include "imagelib/math/Noise.tpp"