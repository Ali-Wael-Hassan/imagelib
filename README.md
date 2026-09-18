# ImageLib

A compact, header-aware C++17 image foundation: core image containers, math,
SIMD, threading, convolution/color primitives, procedural noise, and
STB-backed codecs.

Low-level by design: algorithms operate on normalized `[0,1]` samples through
cheap `ImageView` handles, and the execution hierarchy is
**Scalar → SIMD → Multithreaded → SIMD + Multithreaded** with explicit
selection through `iml::ExecutionPolicy`. SIMD is most effective on contiguous
numeric buffers; interleaved RGB images still pay gather/scatter and
quantization costs.

The 13 classic filters (grayscale, invert, flip, rotate, crop, frame, resize,
blur, edge, oil, ...) are **not** hidden inside the library: they ship as
self-contained kernels in `examples/`. The reports compare a serial baseline,
a SIMD single-thread path where the filter has one, and a parallel path. The
parallel path is normally the fastest because it combines independent rows
with all available cores; SIMD gives its largest gains on contiguous data.

## Features

- **Core** — `Image` / `ImageView` containers, `Types` (pixel/data-type enums,
  format presets), `Buffer`/`Allocator`, typed sample access, error hierarchy.
- **Math** — scalars, `Vec2/3/4`, `Mat2/3/4`, quaternions, geometry,
  interpolation, hashing + seeded `Rng`, and a noise library (white / value /
  gradient / simplex / worley / fractal variants).
- **SIMD** — runtime ISA detection (SSE2/AVX2/AVX-512/NEON/SVE), compile-time
  feature masks, alignment-aware splitting, and reference vector kernels
  (`averageU8`, `saturatingAddU8`, ...).
- **Threading** — `Thread`, `Job`, fixed-size `ThreadPool`, `parallelFor` /
  `parallelForRows[Simd]` / `parallelForColumns[Simd]` with automatic
  serial/parallel dispatch.
- **Benchmarking** — `iml::bench` (`bestMs`, `mpixPerSec`, `BenchReport`): warm
  up, best-of-N, ms + Mpix/s tables with speedup columns.
- **Processing primitives** — the building blocks you write filters with:
  color math (`toGray`, sRGB/linear, HSV/HSL/XYZ/Lab, alpha compositing),
  per-pixel map helpers, and 3×3 `conv::Kernel` convolution with border modes.
- **Procedural** — `fillNoise` (white/value/perlin/simplex/worley/fractal).
- **Codecs** — PNG, JPEG, BMP, TGA through STB (`iml::codecs::registerStbCodec()`
  is auto-registered); thread-safe load/save.

## Layout

```
include/imagelib/     public headers (umbrella: imagelib/imagelib.h)
src/                  library implementation + main
examples/             13 filter demos + example_util.h (benchmark trio)
tutorial/             beginner lessons (.md + .cpp pairs)
external/stb_image/   vendored STB sources (never in public headers)
assets/               sample images for the demos
```

## Build

Requires CMake ≥ 3.15 and a C++17 compiler (Windows: MinGW-w64 tools; the
instructions below use its bundled CMake + `mingw32-make`).

```sh
cmake -G "MinGW Makefiles" -S . -B build
mingw32-make -C build
```

Outputs land in `bin/`:

| Target                  | Description                                 |
|-------------------------|---------------------------------------------|
| `libimagelib.a`         | The library                                 |
| `libstb_image.a`        | STB codec backend (split translation unit)  |
| `imagelib_demo`         | Interactive demo                            |
| `grayscale_demo` … `oil_demo` | 13 filter demos (see **Examples**)   |
| `tutorial_01` … `tutorial_04` | 4 tutorial demos (see **Tutorials**) |

Build configuration knobs (CMake cache):

- `IML_ENABLE_NATIVE` — off by default (portable). Set to `ON` to build with
  `-march=native` and allow wider host-specific instructions such as AVX2.
  Portable x86-64 builds still have the SSE2 baseline.

## Quick start

```cpp
#include "imagelib/imagelib.h"
using namespace iml;

Image src("assets/mario.bmp");                 // load through the registry
Image gray(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);
proc::color::toGray(src.view(), gray.view());  // processing on views
gray.save("out.png");                          // extension selects codec
```

Concurrency:

```cpp
parallelFor(0, (size_t)n, [&](size_t i) { work(i); });
ThreadPool pool(4);
pool.push(Job([]() { work(); }));
pool.waitAll();
```

Writing a filter yourself — read a pixel, do the math, write it back (this is
the pattern every example kernel follows):

```cpp
for (int32 y = 0; y < (int32)src.height(); ++y)
    for (int32 x = 0; x < (int32)src.width(); ++x) {
        const Pixel p = pixel::readPixel(src.view(), x, y);
        const float l = math::clamp01(0.2126f * p.r + 0.7152f * p.g + 0.0722f * p.b);
        pixel::writeNorm(gray.view(), x, y, 0, l);   // Rec.709 grayscale
    }
```

## Conventions

- **Error handling** — functions throw `iml::Error` subclasses
  (`InvalidParameterError`, `InvalidDimensionError`, `UnsupportedFormatError`,
  `InvalidFileError`, `AllocationError`, `IntegerOverflowError`, …).
- **Normalized samples** — point/space/color math uses floats in `[0,1]`; byte
  storage (`UInt8`) is the packed I/O representation.
- **Convolution** is correlation-oriented (no kernel flip).
- **Math** — `Mat2`/`Mat3` rotations follow the library's y-down convention.
- **Portability** — no smart pointers in the core; fixed-width integer types;
  overflow-checked size arithmetic; `alignUp`/`checked` helpers in `iml::mem`.

## Examples — the 13 filters and the benchmark trio

Each of the 13 demos is a single file (`examples/filter_*.cpp`) implementing
one classic filter as its own kernel over the library primitives. Run it and it
executes the *same* kernel three ways and prints the comparison:

1. **`scalar (1 thread)`** — plain nested loops, no SIMD, no threads (the serial
   baseline).
2. **`simd (1 thread)`** — a four-lane kernel where the operation provides a
   useful vector block; interleaved image formats may still be slower because
   gathering and storing samples remains scalar.
3. **parallel / `simd + parallel`** — the filter's multithreaded path. The
   label reflects the implementation: some filters vectorize their inner
   kernel, while point filters use a raw scalar kernel per row to avoid
   expensive RGB gather/scatter.

The multithreaded row is normally the fastest on a large image, but exact
results depend on the filter, image format, CPU, and memory layout. A summary
line quotes the measured speedup:

```
kernel          | pixels |     ms | Mpix/s | vs first
----------------+--------+--------+--------+---------
scalar (1 thread)| 678741 | 26.174 | 25.93  | 1.00x
simd   (1 thread)| 678741 | 19.543 | 34.73  | 1.34x
parallel         | 678741 |  3.441 |197.26  | 7.61x
>> grayscale: "parallel" is the peak at 7.61x of "scalar (1 thread)"
```

Run any demo with no arguments to pick a sample asset, or pass
`[input] [output]`:

```sh
./bin/grayscale_demo.exe  assets/mario.bmp out_gray.png
./bin/blur_demo.exe       assets/mario.bmp out_blur.png 2
./bin/flip_demo.exe       assets/mario.bmp out_flip.png hv
./bin/oil_demo.exe        assets/mario.bmp out_oil.png 3 32
```

A trailing `serial` argument runs only the scalar kernel (no report). Filter
parameters are shown in each file's header or the table below.

| Demo                | Operation                                   | Args after output       |
|---------------------|---------------------------------------------|-------------------------|
| `grayscale_demo`    | Rec.709 luminance                           | —                       |
| `black_white_demo`  | Luminance threshold                         | `threshold`             |
| `invert_demo`       | 1 - pixel                                   | —                       |
| `merge_demo`        | Alpha blend of two images                   | `second.png opacity`    |
| `flip_demo`         | Horizontal / vertical flip                  | `h` / `v` / `hv`        |
| `rotate_demo`       | Rotate 90/180/270 (no resampling)           | `90` / `180` / `270`    |
| `darken_lighten_demo`| Brightness offset                          | `amount`                |
| `crop_demo`         | ROI crop                                    | `x0 y0 w h`             |
| `frame_demo`        | Colored border                              | `thickness r g b`       |
| `edge_demo`         | Sobel gradient magnitude                    | —                       |
| `resize_demo`       | Nearest / bilinear resize                   | `newW newH mode`        |
| `blur_demo`         | Separable Gaussian blur                     | `radius`                |
| `oil_demo`          | Oil-painting (window mode histogram)        | `radius levels`         |

The point-operation kernels preserve pixel-identical results across execution
paths. For genuinely vectorized math, use contiguous channel planes as shown
in `Tutorial 3`, or the `simd::` byte helpers. Interleaved RGB8 point operations
are a useful example of a case where SIMD arithmetic alone does not remove the
cost of channel deinterleaving and output quantization.

## Tutorials

Step-by-step lessons for absolute C++ beginners, each a `.md` lesson plus a
runnable `.cpp` demo built as `tutorial_01_image` … `tutorial_04_simd`. The
tour's example filters are the **grayscale + invert kernels you write by hand**
(Lesson 1) — the library is the tool.

- [01 — Images](tutorial/01_image.md) load/save/inspect, pixels, write your own
  `grayManual` / `invertManual`
- [02 — Math](tutorial/02_math.md) `Vec2/3/4`, `clamp`/`lerp`, matrices, `Rng`
- [03 — Parallel](tutorial/03_parallel.md) every `parallelFor[Rows/Columns][Simd]`
  loop + `ExecutionPolicy`, benchmarked with `iml::bench`
- [04 — SIMD](tutorial/04_simd.md) what SIMD is and the two high-level APIs

See [tutorial/README.md](tutorial/README.md) for the learning path and build
instructions.

## Benchmarking utilities

`include/imagelib/benchmark/Benchmark.h` (`iml::bench`) powers the example
reports:

```cpp
using namespace iml::bench;
double ms = bestMs(5, [&]{ myKernel(src.view(), dst.view()); });
printf("grayscale: %.2f ms (%.1f Mpix/s)\n", ms, mpixPerSec(pixels, ms));

BenchReport report;
report.run("scalar (1 thread)", pixels, 5, 2, [&]{ serialKernel(...); });
report.run("simd + parallel",   pixels, 5, 2, [&]{ fastKernel(...); });
report.print();   // aligned table, ms + Mpix/s + speedup vs first row
```

- `bestMs(iterations, warmup, fn)` — best-of-N ms with explicit warm-up.
- `mpixPerSec(pixels, ms)` — throughput in Mega-pixels/second.
- `BenchReport::run` / `pair` — aligned tables with a `vs first` speedup column
  (and `pair`'s dedicated serial-vs-parallel form).
- `examples/example_util.h::reportTrio` — the shared report helper used by the
  demos. Individual demos choose the label and implementation of their
  multithreaded row; do not assume that every reported parallel path is a
  vectorized RGB gather kernel.
- `reportPoint` — the same trio for any point operator given a scalar and a
  SIMD (Pix4) kernel twin.
