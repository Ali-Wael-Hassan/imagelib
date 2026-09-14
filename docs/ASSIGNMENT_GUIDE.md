# Assignment Guide: writing your own image filters with ImageLib

Welcome! This guide is written for students who are using object-oriented
programming **for the first time** and need to hand in a working image-filter
assignment. It explains the few OOP ideas you actually need, how the ImageLib
library is structured, how to implement each of the 13 required filters from
scratch, and how to benchmark your work.

The library is provided already compiled (`.lib`), so **nothing has to be
deleted or modified** — you add *your own* kernels alongside the provided
examples. Step-by-step templates live in `examples/custom/`.

---

## 1. The only four OOP ideas you need

### 1.1 Objects bundle data + behavior

An `Image` is an object: it *owns* pixel memory and *knows* how to do things with
it (`.load()`, `.save()`, `.width()`, `.set()`, ...). You never manage raw
pointers or `malloc` — the object does that for you.

```cpp
#include "imagelib/imagelib.h"
using namespace iml;

Image img("assets/mario.bmp");   // loads + decodes the file
int w = img.width(), h = img.height();   // asks the object for facts
img.save("copy.png");            // asks the object to encode + write
```

### 1.2 The `const` idea (read-only vs read-write)

`const` tells other programmers (and the compiler!) that a function promises not
to change the input. This is why filters look like this:

```cpp
void myFilter(const ConstImageView& src,  /* read-only input  */
              ImageView          dst);    /* write-only output */
```

If the kernel is correct, `image.view()` (read+write) converts to a
`ConstImageView` automatically, but not the other way around. That catches half
your bugs before the program even runs.

### 1.3 Encapsulation (public interface, hidden internals)

You call `img.save("out.png")`. You don't care *which* codec or *how* it writes
bytes. The object hides all of that behind a clean interface. Your own kernels
should do the same: keep the algorithm inside a function with a clear
signature, and let callers never touch internal details.

### 1.4 Parameters are passed in, not stored globally

A filter that darkens an image takes `float amount` as an argument. The same
function can darken (`amount < 0`) **or** lighten (`amount > 0`) — that is code
reuse. No global variables, no hidden state.

---

## 2. How ImageLib is organized

```
iml::Image          owns the pixels (width, height, channels, data type)
iml::ImageView      lightweight "view" over image memory (no ownership)
iml::ExecutionPolicy  tells algorithms HOW to run:
    .serial()          one thread, scalar code
    .simdParallel()    SIMD + all cores (the fast path used by default)
```

### Normalized samples — the key that makes one kernel work everywhere

Inside a kernel you work with **floats in [0,1]** no matter how the image is
stored (UInt8/UInt16/Float32). Three helpers do all the conversion:

| Helper                          | What it does                              |
|---------------------------------|-------------------------------------------|
| `proc::detail::readNorm(src,x,y,c)`  | read channel `c` of pixel (x,y) as float [0,1] |
| `proc::detail::writeNorm(dst,x,y,c,v)` | write float `v` clamped+converted to the destination type |
| `proc::conv::sampleNorm(src,x,y,c,border)` | like readNorm but with border handling for out-of-range coords |

Why `detail::`? These are the low-level building blocks. You are encouraged to
use them to write your own kernels — that's exactly what the templates do.

### Row-parallel execution (the "optimized systems" part)

Pixels in the same row are independent, and rows are independent from each
other. So the simplest and most effective optimization is: **split the rows
across the thread pool**.

```cpp
auto processRow = [&](uint32 y) {
    for (uint32 x = 0; x < w; ++x)      // your per-pixel math here
        ...
};

if (!iml::detail::wantsParallel(policy, (size_t)w * h, 4096))
    for (uint32 y = 0; y < h; ++y) processRow(y);          // small or serial
else
    parallelForRows(h, [&](size_t y){ processRow((uint32)y); }, policy);
```

- `wantsParallel(policy, pixelCount, threshold)` says: *parallel only if the
  image is big enough to beat the thread-pool overhead*.
- The same `processRow` is reused by both paths — one algorithm, two execution
  strategies. That is the whole optimization story in a nutshell.

Second optimization: the **inner loop is plain float math**, so the compiler
auto-vectorizes it (SSE2 on x86-64, AVX2 if you build with
`-DIML_ENABLE_NATIVE=ON`). Keep heavy stuff — weight tables, sinc lookups —
*outside* the loop.

---

## 3. The 13 filters — implement each from a template

Each template in `examples/custom/` is a complete, compiling, documented kernel.
Open it, read the header comment, then re-type it and make it yours.

| # | Filter | Template | The kernel idea |
|---|--------|----------|-----------------|
| 1 | Grayscale | `custom_grayscale_demo.cpp` | `0.2126r + 0.7152g + 0.0722b` per pixel |
| 2 | Black & white | `custom_black_white_demo.cpp` | luma, then `luma < threshold ? 0 : 1` |
| 3 | Invert | `custom_invert_demo.cpp` | `1.0 - v` per channel |
| 4 | Merge | `custom_merge_demo.cpp` | blend two images by `opacity * alpha` (packed SIMD fast path) |
| 5 | Flip | `custom_flip_demo.cpp` | copy row pixels in reverse / swap row order |
| 6 | Rotate | `custom_rotate_demo.cpp` | inverse-map each dst pixel, bilinear sample |
| 7 | Darken/lighten | `custom_darken_lighten_demo.cpp` | `clamp01(v + amount)` |
| 8 | Crop | `custom_crop_demo.cpp` | copy an axis-aligned rectangle (row copies) |
| 9 | Frame | `custom_frame_demo.cpp` | paint thickness-px border over a copy |
| 10 | Edge detect | `custom_edge_demo.cpp` | Sobel X/Y gradients, output magnitude |
| 11 | Resize | `custom_resize_demo.cpp` | scale mapping + bilinear sample |
| 12 | Blur | `custom_blur_demo.cpp` | separable Gaussian (horizontal + vertical passes) |
| 13 | Oil paint | `custom_oil_demo.cpp` | window histogram, emit the mode per channel |

Run one with an asset image:

```sh
./bin/custom_grayscale_demo.exe assets/mario.bmp my_gray.png
./bin/custom_grayscale_demo.exe assets/mario.bmp my_gray_serial.png serial
```

The second run uses your kernel single-threaded, so you can see how much speed
multithreading gives you.

### Reading order (progressive difficulty)

1. **Point operators** (same pixel in → same pixel out): grayscale → invert →
   darken/lighten → black & white. These introduce `readNorm`/`writeNorm` and
   the `processRow` pattern. Start here.
2. **Copies/reorders**: flip → crop → frame. Introduces `mem::copy` and dirty
   corners of the problem (bounds, dimensions that differ).
3. **Neighborhood/remap operators** (look around (x,y)): resize → rotate →
   blur → edge. Introduces `sampleNorm`, bilinear interpolation and the
   3x3/radius window discipline.
4. **Statistical operator**: oil paint. The only one that uses a histogram and
   a "vote" instead of weighted sums.

---

## 4. How to benchmark (real performance, no headache)

All demos report timings automatically because they go through the shared
benchmark utility `iml::bench`. You can use it directly in your own kernels:

```cpp
#include "imagelib/benchmark/Benchmark.h"
using namespace iml;

// Compare your kernel (serial) against the optimized path:
iml::bench::BenchReport rep;
rep.run("my  kernel (serial)",      pixels, 5, [&]{ myFilter(src.view(), dst.view(), ExecutionPolicy::serial()); });
rep.run("my  kernel (simd+par)",    pixels, 5, [&]{ myFilter(src.view(), dst.view(), ExecutionPolicy::simdParallel()); });
rep.print();
```

Which prints:

```
benchmark                      ms (best)       Mpix/s     vs first
---------------------------------------------------------------
my  kernel (serial)                2.4000        109.4       1.00x
my  kernel (simd+par)              0.3100        847.2       7.74x
```

Rules of thumb you'll need for the write-up:

- Always a **warm-up run** (cache, branches, thread pool settle) — `bestMs`
  does `warmup` runs before measuring, then reports the *best* of N.
- Report **Mpix/s**, not just ms, so kernels of different image sizes compare.
- Real-time target for interactive use is ~33 ms/frame (30 fps). Use
  `BenchReport::runBudget(name, pixels, iterations, 33.3, fn)` to check.
- Don't benchmark while the OS or a browser is busy; close other apps.

---

## 5. Checklist before you hand it in

- [ ] Every kernel works with at least one non-trivial image (use
      `assets/mario.bmp` or `assets/photographer.bmp`).
- [ ] `serial` mode and default (`simdParallel`) mode produce the same output
      (they should be pixel-identical for every filter here).
- [ ] Each kernel reports a time and Mpix/s in the terminal.
- [ ] Nothing from the provided examples was deleted — your kernels coexist.
- [ ] Inputs are passed as `const ConstImageView&` and are never modified.
- [ ] You can explain your `processRow`: *why* it is reusable, and *why* rows
      are the right unit of parallelism.
- [ ] You can name your two optimizations (row-parallelism + vectorizable
      inner loop) and the trade-off (`wantsParallel` threshold).