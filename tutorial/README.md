# ImageLib Tutorials

A beginner-friendly tour of ImageLib, written for people who are learning C++
**and** want to use a real image library the way it is used in production —
not the deep internals (allocators, codec formats, SIMD intrinsics), but the
part you actually call in your own programs.

Every topic ships as a pair of files:

| File              | What it is                                                     |
|-------------------|----------------------------------------------------------------|
| `*.md`            | The lesson: ideas explained slowly, with short code snippets.  |
| `*.cpp`           | The demo: one complete, runnable program illustrating the lesson. |

You can read a `.md`, then run its `.cpp` and watch it do the thing on a real
image.

## Learning path

	1. Images        -> tutorial/01_image.md       (+ 01_image.cpp)
	2. Math          -> tutorial/02_math.md        (+ 02_math.cpp)
	3. Parallel      -> tutorial/03_parallel.md    (+ 03_parallel.cpp)
	4. SIMD          -> tutorial/04_simd.md        (+ 04_simd.cpp)

Read them in order. Each lesson only uses concepts from the previous one.
Lesson 3 also doubles as your introduction to the benchmarking utilities in
`imagelib/benchmark/` (see `bench::bestMs` and `bench::BenchReport`).

| Lesson | You will learn to do                                  |
|--------|-------------------------------------------------------|
| 01 Images | Load, inspect, save pictures; read/write pixels; write your own **grayscale + invert** by hand (the library is the *tool* — this is the shape every assignment filter takes). |
| 02 Math   | `Vec2/3/4`, lengths, dot products, `clamp`/`lerp`, angles, random `Rng` — used to *draw* and shade images. |
| 03 Parallel | Every loop in the `parallelFor` family (`parallelFor`, `parallelForRows`, `parallelForColumns` and their SIMD twins), run on a real photo with the hand-written gray + invert kernels, plus how to benchmark them with `iml::bench`. |
| 04 SIMD     | What "SIMD" means, what ImageLib does for you automatically, and the two high-level APIs you may touch. |

The example filters through the whole tour are the **grayscale** and **invert**
kernels you write yourself in Lesson 1. The other assignment filters (flip,
rotate, crop, frame, edge, resize, blur, oil, ...) ship as the 13 performance
demos in `../../examples/` — run those for the completed, benchmarked versions
and to compare serial, SIMD, and multithreaded execution on the same image.
For interleaved RGB images, the multithreaded path can win by more than the
single-thread SIMD path because SIMD arithmetic does not eliminate channel
gather/scatter and byte quantization.

## What you need

- A C++17 compiler (the repo is set up for MinGW-w64 + `mingw32-make`).
- A build of the library. From the repository root:

```sh
cmake -G "MinGW Makefiles" -S . -B build
mingw32-make -C build
```

That puts `libimagelib.a` and the demo programs into `bin/`.

## How to run a tutorial demo

If you built through CMake, each `tutorial_XX_*` target is built for you:

```sh
./bin/tutorial_01_image.exe
./bin/tutorial_02_math.exe
./bin/tutorial_03_parallel.exe
./bin/tutorial_04_simd.exe
```

Or compile a single lesson by hand against the built library:

```sh
g++ -std=c++17 -O2 -Iinclude -Isrc tutorial/01_image.cpp \
    -Lbin -limagelib -lstb_image -o bin/tutorial_01_image.exe
```

All demos write their output images into the current working directory, so
run them from a scratch folder or clean up afterwards. The sample pictures
used by the demos live in `../../assets/` relative to the repository root
(e.g. `assets/mario.bmp`). `03_parallel` prefers a larger real photograph
(`assets/mario.jpg`, `assets/toy2.jpg`, ...) so the speedup is visible; every
demo falls back to `assets/mario.bmp` if you do not pass a path.

## One file to read first

The umbrella header `include/imagelib/imagelib.h` pulls in the entire public
API. Every tutorial program starts with:

```cpp
#include "imagelib/imagelib.h"
using namespace iml;
```

Everything the demos use lives behind that one `#include`.