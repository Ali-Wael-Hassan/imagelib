# Tutorial 2 — Math for Images

**Demo:** `02_math.cpp` — it prints what the math helpers do, then uses them
to paint a shaded circle and a random-noise image.

Image code is mostly *numbers*. Position, distance, color blends, angles and
noise all show up in even the simplest filter. ImageLib's `iml::math`
namespace is the small subset that production image code actually reaches for.

## 2.1 Vectors: one value per axis

A `Vec2` is just two floats — normally `x` and `y`. `Vec3` adds `z`, `Vec4`
adds `w` (used a lot as **RGBA colors**).

```cpp
using namespace iml;          // Vec2 etc. live in iml::math
using namespace iml::math;

Vec2 pos(100.f, 240.f);       // a pixel position
Vec2 speed(2.f, 0.f);         // a direction

pos = pos + speed;            // +,-,*,/ work component-wise
float d = pos.length();       // distance from (0,0)
Vec2 unit = pos.normalized(); // same direction, length 1
```

Useful free functions (also members):

| Expression                | Meaning                                  |
|---------------------------|------------------------------------------|
| `distance(a, b)`          | distance between two points              |
| `dot(a, b)`               | a.x*b.x + a.y*b.y; 0 ⇔ perpendicular     |
| `cross(a, b)` (Vec3)      | perpendicular vector (normal)            |
| `normalize(v)`            | unit vector in v's direction             |
| `clampComponentwise(v, lo, hi)`, `minElementWise`, `maxElementWise` | safe per-channel math |

**Vector math for pixels:** the distance to a shape's center tells you if a
pixel is inside it. That is the whole of drawing circles, radial gradients,
and glow effects:

```cpp
float d = Vec2(x, y).distance(center);
float t = smoothstep(radius - edge, radius + edge, d);  // 0 inside, 1 outside
```

## 2.2 Scalar helpers: the safety functions

The library uses values in **[0, 1]** everywhere (see Tutorial 1), and
math gives you the tools to keep them there:

```cpp
float v = clamp(n, 0.f, 1.f);        // hard clamp
float s = saturate(n);               // clamp to [0,1] for floats
float m = lerp(0.f, 1.f, t);         // a + (b - a) * t, smooth blend
float r = remap(0.5f, 0.f, 1.f, 10.f, 20.f);   // map one range to another
```

`smoothstep` and its smoother cousin `smootherstep` are the "make it fade
softly" functions — every blur/gradient/dissolve in the demos is built from
them. `toRadians(90.f)` / `toDegrees(x)` convert angles; constants like
`pi`, `halfPi`, `sqrt2` are there too.

## 2.3 Matrices: rotate and scale coordinates

A `Mat2`/`Mat3` transforms vectors. You create one with a factory, then
multiply a vector:

```cpp
Mat2 rot = Mat2::rotation(toRadians(45.f));
Vec2 turned = rot * Vec2(1.f, 0.f);        // (0.707, -0.707): see note below

Mat2 sc = Mat2::scale(Vec2(2.f, 3.f));
Vec2 grown = sc * Vec2(1.f, 1.f);          // (2, 3)

Mat3 m = Mat3::identity();                 // the "do nothing" matrix
```

Elements are indexed `m(col, row)` because the storage is **column-major**
(GLM convention). One convention quirk worth knowing: images grow **down**,
so rotation matrices follow the library's **y-down** rule and a positive
angle rotates *clockwise on screen* — turning `(1,0)` by 45° gives
`(0.7071, -0.7071)`. That is intentional and consistent across the library.

## 2.4 Random numbers: `Rng`

`math::Rng` is a tiny, fast, deterministic random engine. "Deterministic"
means: **same seed → same sequence**. That is a feature — tests and
reproducible renders.

```cpp
Rng rng(42u);                 // pick any seed
float a = rng.nextFloat01();  // uniform in [0,1]
int   d = rng.nextInt(1, 6);  // dice
Vec2  v = rng.nextVec2();     // random vector in the unit square
```

Making noise for an image is three lines:

```cpp
Image noise(fmt::gray8, w, h, ColorSpace::Gray);
Rng rng(2026u);
pixel::writeNorm(noise.view(), x, y, 0, rng.nextFloat01());
```

(`writeNorm` stores any float in [0,1] into whatever the image's type is —
8-bit or 16-bit, it just works.)

## 2.5 Putting math and images together

The demo's `drawCircle()` builds the whole picture from the pieces above:

1. allocate an `Image(fmt::rgba8, 512, 512)` with `SRGB` color space;
2. for every `(x, y)`, compute `d = Vec2(x,y).distance(center)`;
3. turn the distance into a blend factor with `smoothstep`;
4. blend two colors with `mix()` (`Pixel a * (1-t) + Pixel b * t`);
5. `pixel::writeRGBA(p, view, x, y)` stores it.

That one per-pixel idea — *"position in, color out"* — powers procedural
textures, heightmaps, heat-maps, and a good chunk of image processing.

## 2.6 Summary

| You want to...      | You write                                  |
|---------------------|--------------------------------------------|
| A position/color    | `Vec2`, `Vec3`, `Vec4` (`Vec4` doubles as RGBA) |
| A safe number       | `clamp(x, 0, 1)`, `saturate(x)`            |
| Smooth fades        | `lerp(a, b, t)`, `smoothstep(a, b, x)`     |
| Angles              | `toRadians`, `toDegrees`, `pi`             |
| Rotate/scale a point | `Mat2::rotation(a) * v`, `Mat2::scale(s) * v` |
| Random values       | `Rng rng(seed); rng.nextFloat01();`        |
| Compute pixel color | `d = distance(...); t = smoothstep(...); p = mix(c0, c1, t);` |

Next: [Tutorial 3 — Parallel](03_parallel.md) — the one-line change that
makes a slow per-pixel loop use all your CPU cores.