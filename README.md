# ImageLib

A compact, header-aware C++17 image foundation: core image containers, math,
SIMD, threading, packing/quantization, image processing, procedural generation,
and STB-backed codecs.

Low-level by design: algorithms operate on normalized `[0,1]` samples through
cheap `ImageView` handles, and the execution hierarchy is
**Scalar → SIMD → Multithreaded → SIMD + Multithreaded** with automatic dispatch
through `iml::ExecutionPolicy`.

## Features

- **Core** — `Image` / `ImageView` containers, `Types` (pixel/data-type enums,
  format presets), `Buffer`/`Allocator`, typed sample access, error hierarchy.
- **Math** — scalars, `Vec2/3/4`, `Mat2/3/4`, quaternions, geometry, interpolation,
  hashing + seeded `Rng`, and a noise library (white / value / gradient / simplex /
  worley / cellular + fractal variants).
- **SIMD** — runtime ISA detection (SSE2/AVX2/AVX-512/NEON/SVE), compile-time
  feature masks, alignment-aware splitting, and reference vector kernels.
- **Threading** — `Thread`, `Job`, fixed-size `ThreadPool`, `parallelFor` /
  `parallelForRows` with automatic serial/parallel dispatch.
- **Compression** — `BitWriter`/`BitReader`, packed pixel formats (1/2/4/8/16-bit,
  RGB565, RGBA4444/5551, …), palette building, uniform + Floyd–Steinberg
  quantization, `PackedImage` container.
- **Processing** — pixel ops, color models (sRGB/linear, HSV, HSL, Lab, alpha
  compositing), convolution and separable Gaussian/box blur, Sobel edge kernels,
  filters (invert, brightness, contrast, threshold, median), nearest/bilinear
  resize, flips/rotations/transpose/ROI, morphology (dilate/erode/open/close),
  analysis (min/max, mean/stddev, histogram, Otsu threshold, entropy).
- **Procedural** — `fillNoise` (white/value/perlin/simplex/worley/fractal combos),
  gradients/circle/ring/checkerboard masks, heightmap→normal/shade/contour,
  wood/marble/clouds/plasma textures.
- **Codecs** — PNG, JPEG, BMP, TGA through STB (`iml::codecs::registerStbCodec()`
  is auto-registered); thread-safe load/save.

## Layout

```
include/imagelib/     public headers (umbrella: imagelib/imagelib.h)
src/                  library implementation + demo main
tests/                self-contained test suite (no framework dependency)
benchmarks/           coarse kernel timings
examples/             small standalone demos
external/stb_image/   vendored STB sources (never in public headers)
assets/               sample images for the demos
```

## Build

Requires CMake ≥ 3.15 and a C++17 compiler (Windows: MinGW-w64 tools; the
instructions below use its bundled CMake + `mingw32-make`).

```sh
cmake -G "MinGW Makefiles" -S . -B build
mingw32-make -C build
ctest --test-dir build --output-on-failure   # run the test suite
```

Outputs land in `bin/`:

| Target                  | Description                                 |
|-------------------------|---------------------------------------------|
| `libimagelib.a`         | The library                                 |
| `libstb_image.a`        | STB codec backend (split translation unit)  |
| `imagelib_tests`        | Test suite (all subsystems)                 |
| `imagelib_bench`        | Benchmark driver (`imagelib_bench [runs]`)  |
| `imagelib_demo`         | Interactive grayscale demo                  |
| `basic_demo`            | Batch grayscale converter example           |
| `processing_demo`       | Blur/edge/procedural/heightmap example      |
| `<filter>_demo`         | 13 image-operation demos (see Examples)     |
| `custom_<filter>_demo`  | 13 teaching demos that reimplement kernels  |

Build configuration knobs (CMake cache):

- `IML_ENABLE_NATIVE` — off by default (portable). Set to `ON` to build with
  `-march=native` and enable the SIMD dispatch fast paths.

## Quick start

```cpp
#include "imagelib/imagelib.h"
using namespace iml;

Image src("assets/mario.bmp");                 // load through the registry
Image gray(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);
proc::color::toGray(src.view(), gray.view());  // processing on views
gray.save("out.png");                          // extension selects codec
```

Processing functions write into caller-owned destination views:

```cpp
Image blurred(fmt::gray32f, w, h, ColorSpace::Gray);
proc::conv::gaussianBlur(gray.view(), blurred.view(), 2.0f);

Image gx(fmt::gray32f, w, h, ColorSpace::Gray);
proc::conv::convolve(gray.view(), gx.view(), proc::conv::sobelXKernel());
```

Concurrency:

```cpp
parallelFor(0, (size_t)n, [&](size_t i) { work(i); });
ThreadPool pool(4);
pool.push(Job([]() { work(); }));
pool.waitAll();
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

## Tests & benchmarks

`ctest` runs a single `imagelib_tests` binary that prints per-subsystem results:

```
[OK  ] types        [OK  ] memory        [OK  ] math
[OK  ] image        [OK  ] compression   [OK  ] simd
[OK  ] threading    [OK  ] processing    [OK  ] procedural
[OK  ] codecs
ALL TESTS PASSED
```

Every subsystem is also validated on its *optimized* paths: the processing,
noise and quantization tests compare serial vs `ExecutionPolicy::simdParallel()`
outputs, and the codec test round-trips a large image through the multithreaded
load/save pipeline.

## Examples

The 13 example binaries cover the classic image operations against `assets/`
images. Each prints best-of-N timings in ms + Mpix/s via the shared benchmark
utility, with a `serial` argument to toggle the multithreaded backend:

```sh
./bin/invert_demo.exe assets/mario.bmp inverted.png
./bin/invert_demo.exe assets/mario.bmp inverted-serial.png serial
```

| Demo                | Operation                                   |
|---------------------|---------------------------------------------|
| `grayscale_demo`    | Rec.709 luminance                           |
| `black_white_demo`  | Luminance threshold                         |
| `invert_demo`       | 1 - pixel                                   |
| `merge_demo`        | Alpha blend of two images                   |
| `flip_demo`         | Horizontal / vertical flip                  |
| `rotate_demo`       | Rotate 90/180/270 (no resampling)           |
| `darken_lighten_demo`| Brightness offset (`serial`-aware)         |
| `crop_demo`         | ROI crop                                    |
| `frame_demo`        | Colored border                              |
| `edge_demo`         | Sobel gradient magnitude                    |
| `resize_demo`       | Nearest / bilinear resize                   |
| `blur_demo`         | Separable Gaussian blur                     |
| `oil_demo`          | Oil-painting (window mode histogram)        |

### Reimplementing the filters yourself

`examples/custom/custom_<filter>_demo.cpp` teaches how to write every one of
these kernels from scratch — a small documented kernel per file, using only
`readNorm`/`writeNorm`, `sampleNorm` and `parallelForRows`, all driven through
`ExecutionPolicy`. This is the intended starting point for the assignment (see
[docs/ASSIGNMENT_GUIDE.md](docs/ASSIGNMENT_GUIDE.md)); the library ships
prebuilt so nothing has to be deleted to hand in your own kernels.

## Benchmarking utilities

`include/imagelib/benchmark/Benchmark.h` (`iml::bench`) is the benchmark layer
used by the driver, the demos and the docs:

```cpp
using namespace iml::bench;
double ms = bestMs(5, [&]{ proc::filter::invert(src.view(), dst.view()); });
printf("invert: %.2f ms (%.1f Mpix/s)\n", ms, mpixPerSec(pixels, ms));

BenchReport report;
report.run("serial",      pixels, 5, [&]{ SlowKernel(...); });
report.run("simd+par",    pixels, 5, [&]{ FastKernel(...); });
report.print();   // aligned table, ms + Mpix/s + speedup vs first row
```

- `bestMs(iterations, warmup, fn)` — best-of-N ms with explicit warm-up.
- `mpixPerSec(pixels, ms)` — throughput in Mega-pixels/second.
- `BenchReport::runBudget(name, pixels, iterations, budgetMs, fn)` — flags
  kernels that miss a real-time budget (e.g. 33.3 ms / frame at 30 fps).

`imagelib_bench [runs]` benches the shipped kernels end-to-end (Sobel convolve,
separable Gaussian blur, bilinear resize up/down, Perlin/domain-warp fill,
quantization, to-gray, invert, thread-pool task throughput).