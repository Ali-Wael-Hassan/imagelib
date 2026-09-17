#pragma once
#define IMAGELIB_MATH_NOISE_H_
/// @file
/// Generic mathematical noise: Value, Perlin, Simplex, Worley/Cellular, White
/// and Gradient, plus fractal combinators. Pure math, no image dependency.

#include "imagelib/core/Types.h"
#include "imagelib/math/Scalar.h"
#include "imagelib/math/Random.h"

#include <cmath>

namespace iml {
namespace math {
namespace noise {

/// Deterministic, seed-dependent hash of up to four lattice coordinates.
/// @param seed Hash seed.
/// @param a First lattice coordinate.
/// @param b Second lattice coordinate (default 0).
/// @param c Third lattice coordinate (default 0).
/// @param d Fourth lattice coordinate (default 0).
/// @return Hash of the coordinates.
uint64 latticeHash(uint64 seed, int64 a, int64 b = 0, int64 c = 0, int64 d = 0) noexcept;

/// Deterministic, seed-dependent hash of seven lattice coordinates.
/// @param seed Hash seed.
/// @param a First lattice coordinate.
/// @param b Second lattice coordinate.
/// @param d Third lattice coordinate.
/// @param e Fourth lattice coordinate.
/// @param f Fifth lattice coordinate.
/// @param g Sixth lattice coordinate.
/// @return Hash of the coordinates.
uint64 latticeHash(uint64 seed, int64 a, int64 b, int64 d, int64 e, int64 f, int64 g) noexcept;

/// Maps a hash to a unit value in [0, 1].
/// @param h Hash value.
/// @return Value in [0, 1].
float unit(uint64 h) noexcept;

/// Quintic fade curve for lattice interpolation.
/// @param t Interpolation coordinate.
/// @return Faded value.
float fade(float t) noexcept;

/// Smoothly interpolates with the quintic fade.
/// @param a First value.
/// @param b Second value.
/// @param t Interpolation parameter in [0, 1].
/// @return Interpolated value.
float fadeLerp(float a, float b, float t) noexcept;

/// Table of 2D unit gradient vectors for Perlin-style noise.
constexpr float grad2Table[8][2] = {
    {1, 1},
    {-1, 1},
    {1, -1},
    {-1, -1},
    {1, 0},
    {-1, 0},
    {0, 1},
    {0, -1},
};

/// Table of 3D unit gradient vectors for Perlin-style noise.
constexpr float grad3Table[16][3] = {
    {1, 1, 0},
    {-1, 1, 0},
    {1, -1, 0},
    {-1, -1, 0},
    {1, 0, 1},
    {-1, 0, 1},
    {1, 0, -1},
    {-1, 0, -1},
    {0, 1, 1},
    {0, -1, 1},
    {0, 1, -1},
    {0, -1, -1},
    {1, 1, 0},
    {-1, 1, 0},
    {0, -1, 1},
    {0, -1, -1},
};

/// Dot product of a gradient from the 2D table with (x, y).
/// @param h Hash selecting a gradient.
/// @param x X coordinate.
/// @param y Y coordinate.
/// @return Gradient dot product.
float dotGrad2(uint64 h, float x, float y) noexcept;
/// Dot product of a gradient from the 3D table with (x, y, z).
/// @param h Hash selecting a gradient.
/// @param x X coordinate.
/// @param y Y coordinate.
/// @param z Z coordinate.
/// @return Gradient dot product.
float dotGrad3(uint64 h, float x, float y, float z) noexcept;

/// White noise, 1D: uncorrelated sample per lattice cell.
/// @param seed Noise seed.
/// @param x Sample coordinate.
/// @return Value in [0, 1].
float whiteNoise1D(uint64 seed, float x) noexcept;
/// White noise, 2D: uncorrelated sample per lattice cell.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @return Value in [0, 1].
float whiteNoise2D(uint64 seed, float x, float y) noexcept;
/// White noise, 3D: uncorrelated sample per lattice cell.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param z Sample z.
/// @return Value in [0, 1].
float whiteNoise3D(uint64 seed, float x, float y, float z) noexcept;
/// White noise, 4D: uncorrelated sample per lattice cell.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param z Sample z.
/// @param w Sample w.
/// @return Value in [0, 1].
float whiteNoise4D(uint64 seed, float x, float y, float z, float w) noexcept;

/// Value noise, 1D: interpolated lattice hashes.
/// @param seed Noise seed.
/// @param x Sample coordinate.
/// @return Value in [0, 1].
float valueNoise1D(uint64 seed, float x) noexcept;
/// Value noise, 2D: interpolated lattice hashes.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @return Value in [0, 1].
float valueNoise2D(uint64 seed, float x, float y) noexcept;
/// Value noise, 3D: interpolated lattice hashes.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param z Sample z.
/// @return Value in [0, 1].
float valueNoise3D(uint64 seed, float x, float y, float z) noexcept;
/// Value noise, 4D: interpolated lattice hashes.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param z Sample z.
/// @param w Sample w.
/// @return Value in [0, 1].
float valueNoise4D(uint64 seed, float x, float y, float z, float w) noexcept;

/// Gradient noise, 1D: Perlin-style gradient lattice.
/// @param seed Noise seed.
/// @param x Sample coordinate.
/// @return Value in [0, 1].
float gradientNoise1D(uint64 seed, float x) noexcept;
/// Gradient noise, 2D: Perlin-style gradient lattice.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @return Value in [0, 1].
float gradientNoise2D(uint64 seed, float x, float y) noexcept;
/// Gradient noise, 3D: Perlin-style gradient lattice.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param z Sample z.
/// @return Value in [0, 1].
float gradientNoise3D(uint64 seed, float x, float y, float z) noexcept;
/// Gradient noise, 4D: Perlin-style gradient lattice.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param z Sample z.
/// @param w Sample w.
/// @return Value in [0, 1].
float gradientNoise4D(uint64 seed, float x, float y, float z, float w) noexcept;

/// Perlin noise, 1D: classic gradient lattice noise.
/// @param seed Noise seed.
/// @param x Sample coordinate.
/// @return Value in [0, 1].
float perlinNoise1D(uint64 seed, float x) noexcept;
/// Perlin noise, 2D: classic gradient lattice noise.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @return Value in [0, 1].
float perlinNoise2D(uint64 seed, float x, float y) noexcept;
/// Perlin noise, 3D: classic gradient lattice noise.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param z Sample z.
/// @return Value in [0, 1].
float perlinNoise3D(uint64 seed, float x, float y, float z) noexcept;
/// Perlin noise, 4D: classic gradient lattice noise.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param z Sample z.
/// @param w Sample w.
/// @return Value in [0, 1].
float perlinNoise4D(uint64 seed, float x, float y, float z, float w) noexcept;

/// Simplex noise, 2D.
/// @param seed Noise seed.
/// @param xin Sample x.
/// @param yin Sample y.
/// @return Value in [0, 1].
float simplexNoise2D(uint64 seed, float xin, float yin) noexcept;
/// Simplex noise, 3D.
/// @param seed Noise seed.
/// @param xin Sample x.
/// @param yin Sample y.
/// @param zin Sample z.
/// @return Value in [0, 1].
float simplexNoise3D(uint64 seed, float xin, float yin, float zin) noexcept;

/// 2D Worley/cellular noise sample.
struct Worley2D {
    /// Distance to the nearest feature point.
    float f1;
    /// Distance to the second nearest feature point.
    float f2;
    /// Offset of the nearest feature point within its cell.
    Vec2 closest;
    /// Returns the value scaled to [0, 1].
    /// @return Scaled value.
    float value() const noexcept;
    /// Returns the F2-F1 "crackle" edge amount.
    /// @return Edge amount.
    float edge() const noexcept;
};

/// 2D cellular noise over the unit cell; returns distances, not colors.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @return Worley sample.
Worley2D worley2D(uint64 seed, float x, float y) noexcept;

/// 3D Worley/cellular noise sample.
struct Worley3D {
    /// Distance to the nearest feature point.
    float f1;
    /// Distance to the second nearest feature point.
    float f2;
    /// Returns the value scaled to [0, 1].
    /// @return Scaled value.
    float value() const noexcept;
    /// Returns the F2-F1 "crackle" edge amount.
    /// @return Edge amount.
    float edge() const noexcept;
};

/// 3D cellular noise over the unit cell; returns distances, not colors.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param z Sample z.
/// @return Worley sample.
Worley3D worley3D(uint64 seed, float x, float y, float z) noexcept;

/// Controls the stacking of fractal noise octaves.
struct FractalParams {
    /// Number of octaves.
    int octaves = 4;
    /// Base lattice frequency.
    float frequency = 1.0f;
    /// Frequency multiplier per octave.
    float lacunarity = 2.0f;
    /// Amplitude multiplier per octave.
    float persistence = 0.5f;
    /// Amplitude of the first octave (value noise formula).
    float gain = 1.0f;
    /// Ridge offset for ridged noise.
    float ridgeOffset = 1.0f;
};

/// Fractional Brownian motion over a binary noise function.
/// `baseFn(seed, x, y)` returns base noise such that the FBM in [-1,1] space.
/// @tparam Fn Callable type `float Fn(uint64, float, float)`.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param p Fractal parameters.
/// @param baseFn Base noise function.
/// @return Fractal sample in [-1, 1].
template <typename Fn>
float fbmGeneric(uint64 seed, float x, float y, const FractalParams& p, Fn baseFn) noexcept;

/// FBM built on gradient/perlin 2D (returns base noise ~[-1,1]).
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param p Fractal parameters.
/// @return Fractal sample.
float fbm2D(uint64 seed, float x, float y, const FractalParams& p = {}) noexcept;

/// 3D FBM built on gradient/perlin noise.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param z Sample z.
/// @param p Fractal parameters.
/// @return Fractal sample.
float fbm3D(uint64 seed, float x, float y, float z, const FractalParams& p = {}) noexcept;

/// Ridged multifractal: octaves inverted on |noise| so ridges crest sharply.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param p Fractal parameters.
/// @return Ridged fractal sample.
float ridged2D(uint64 seed, float x, float y, const FractalParams& p = {}) noexcept;

/// Billow: abs() lobes still leave soft peaks (classic "pillow").
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param p Fractal parameters.
/// @return Billow sample.
float billow2D(uint64 seed, float x, float y, const FractalParams& p = {}) noexcept;

/// Hybrid multifractal mixing fbm and ridged octaves.
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param p Fractal parameters.
/// @return Hybrid fractal sample.
float hybrid2D(uint64 seed, float x, float y, const FractalParams& p = {}) noexcept;

/// Domain warping: sample noise at coordinates displaced by two lower-octave
/// noise fields (classic "stretched/smudged" manifolds).
/// @param seed Noise seed.
/// @param x Sample x.
/// @param y Sample y.
/// @param amount Warp amount.
/// @param wp Warp fractal parameters.
/// @return Warped noise sample.
float domainWarp2D(
    uint64 seed,
    float x,
    float y,
    float amount = 1.0f,
    const FractalParams& wp = {}) noexcept;

} // namespace noise
} // namespace math
} // namespace iml

#include "math/Noise.tpp"