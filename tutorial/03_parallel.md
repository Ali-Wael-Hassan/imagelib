# Tutorial 3 — The Complete `parallelFor` Family

**Demo:** `03_parallel.cpp` — it loads a **real photo** from `assets/`, runs
the hand-written grayscale and invert kernels from Tutorial 1 through
**every** `parallelFor` function the library has, saves the results, and then
benchmarks the fastest ones with the library's `iml::bench` utilities.

The classic filter writes every output pixel from some input pixels. Those
writes are **independent** — pixel (0,0) does not care about pixel (100,100) —
so nothing stops you from doing several at once. That is all parallelism in
image processing is.

ImageLib gives you **six** loops. Three run one scalar task per index
(row / column / anything), three batch *several pixels at once* on SIMD
registers. Below, every single one is explained parameter by parameter, then
used on the photo — not on a synthetic gradient. The filters are the gray and
invert kernels you already wrote in Tutorial 1, so nothing here is about the
filter — it is about the *loop* you run it with.

---

## 3.1 The four execution strategies (`ExecutionPolicy`)

Every loop takes the same trailing argument: an `ExecutionPolicy` that says
*how* to run. Policy is always the last parameter.

```cpp
ExecutionPolicy::serial();        // 1 thread, scalar math
ExecutionPolicy::parallel();      // all threads, scalar math
ExecutionPolicy::simd();          // 1 thread, SIMD instructions
ExecutionPolicy::simdParallel();  // all threads + SIMD  (the "fast" mode)
ExecutionPolicy()                 // Auto: the library picks (see below)
```

The default (`ExecutionPolicy()`) is `Auto`: the library only parallelizes when
the work count is big enough to beat the overhead of waking up worker threads.
For SIMD range loops, the decision uses the total element/pixel count rather
than only the number of vector blocks. Two things to remember:

- For `parallelForRows`, the "items" are **rows**, not pixels. A 1024-tall
  image has 1024 rows, which is below the threshold, so `Auto` keeps it
  serial. Always pass an **explicit** policy when you want parallelism; the
  shipped filters do, and so does this tutorial.
- `parallelForRowsSimd` and `parallelForColumnsSimd` use the full image work
  count for their automatic decision, then split rows or columns into
  `Vec::Lanes`-wide blocks.
- `iml::detail::wantsParallel(policy, count, threshold)` is the decision helper
  behind all of this. The threshold inline style you see in production code:

```cpp
if (iml::detail::wantsParallel(policy, (size_t)w * h, 4096)) {
    parallelForRows(h, lambda, policy);      // big picture -> thread pool
} else {
    for (uint32 y = 0; y < h; ++y) lambda(y);  // small picture -> plain loop
}
```

---

## 3.2 The three *scalar* loops

### `parallelForRows(height, fn, policy)`

One task per **row**. Rows never overlap, so workers never fight over memory.
This is the loop you reach for 90% of the time for image filters.

| Parameter | What it means |
|-----------|---------------|
| `height`  | Number of rows — `fn` is called once per row index in `[0, height)`. Pass `img.height()`. |
| `fn`      | Called as `fn(size_t row)`. Inside, loop over `x` yourself and write the whole row. |
| `policy`  | Which `ExecutionPolicy` to run under (last argument, optional → `Auto`). |

Real example — a grayscale filter over a real photo (the luma math is the
`grayPixel` kernel from Tutorial 1):

```cpp
void grayRows(const ConstImageView& src, ImageView dst, const ExecutionPolicy& policy) {
    const int32 w = (int32)src.width();
    parallelForRows(
        (size_t)src.height(),
        [&](size_t row) {                       // "row" is the only thing fn gets
            const int32 y = (int32)row;
            for (int32 x = 0; x < w; ++x) {
                const Pixel p = pixel::readPixel(src, x, y);
                const float l = math::clamp01(0.2126f * p.r + 0.7152f * p.g + 0.0722f * p.b);
                pixel::writeRGBA(Pixel(l, l, l, 1.f), dst, x, y);
            }
        },
        policy);
}
```

### `parallelForColumns(width, fn, policy)`

One task per **column**. Same shape, just the loop is over `col` and you walk
`y` yourself:

| Parameter | What it means |
|-----------|---------------|
| `width`   | Number of columns — `fn` is called once per column index in `[0, width)`. Pass `img.width()`. |
| `fn`      | Called as `fn(size_t col)`. Inside, loop over `y` yourself. |
| `policy`  | Which `ExecutionPolicy` to run under. |

Real example — invert a photo, one column per task (each column's pixels are
independent of every other column's):

```cpp
void invertColumns(const ConstImageView& src, ImageView dst, const ExecutionPolicy& policy) {
    parallelForColumns(
        (size_t)src.width(),
        [&](size_t col) {
            for (int32 y = 0; y < (int32)src.height(); ++y) {
                const Pixel p = pixel::readPixel(src, (int32)col, y);
                pixel::writeRGBA(Pixel(1.f - p.r, 1.f - p.g, 1.f - p.b, p.a), dst, (int32)col, y);
            }
        },
        policy);
}
```

### `parallelFor(begin, end, fn, policy)` and its `(n, fn, policy)` shortcut

For anything that is **not** naturally rows or columns — a plain 1D range of
independent work. `fn` is called with indices in **an unspecified order**.

| Parameter | What it means |
|-----------|---------------|
| `begin`   | First index. Also present only here: every full-array call becomes `parallelFor(0, n, ...)`. |
| `end`     | One past the last index (so `fn(i)` runs for `i` in `[begin, end)`). |
| `n`       | Shortcut overload: `parallelFor(n, ...)` ≡ `parallelFor(0, n, ...)`. |
| `fn`      | Called as `fn(size_t i)`. |
| `policy`  | Which `ExecutionPolicy` to run under. |

Real example — invert a photo by its **linear pixel index** (one index per
pixel, row-major). Each index writes only its own pixel, so it is safe:

```cpp
const size_t n = (size_t)src.width() * src.height();
parallelFor(n,
    [&](size_t i) {
        const int32 x = (int32)(i % src.width());   // index -> (x, y)
        const int32 y = (int32)(i / src.width());
        const Pixel p = pixel::readPixel(src, x, y);
        pixel::writeRGBA(Pixel(1.f - p.r, 1.f - p.g, 1.f - p.b, p.a), dst, x, y);
    },
    ExecutionPolicy::parallel());
```

Prefer `parallelForRows`/`parallelForColumns` for images (better cache
behaviour), and keep `parallelFor` for arrays, look-up tables, and for
**processing only part** of an image — the `[begin, end)` form is perfect for
that (the demo grayscales only the left half of the photo).

---

## 3.3 Safety rule before we go faster

This one rule keeps you out of trouble with **all six** loops:

> Index `i` may write its own output slot `out[i]`, and may read anything.
> It must NOT write a slot another index also writes, unless you protect it.

So `out[i] = f(in[i])` is fine; `total += arr[i]` is **not** fine (several
workers would update `total` at once). Collect values in the array first,
then combine them afterwards with a single-threaded loop.

---

## 3.4 The three *SIMD* loops

These take **two** callbacks instead of one. The library splits the work into
`Vec::Lanes`-wide blocks, calls `simdFn` once per block, and calls `scalarFn`
once for each leftover ("tail") element that does not fill a whole block.
A row of 130 pixels with 4 lanes → 32 blocks (128 pixels) + 2 tail pixels.

`Vec` is any `simd::SimdVec`; the common one is `simd::Float4` (4 floats =
4 pixels' worth of one channel). Full SIMD detail is Tutorial 4 — here you
only need the loop signature and what to put in each lambda.

### `parallelForRowsSimd<Vec>(width, height, simdFn, scalarFn, policy)`

The workhorse. **Note the argument order: `width` first, then `height`.**

| Parameter | What it means |
|-----------|---------------|
| `Vec`     | SIMD container deciding block width. `simd::Float4` = 4 pixels per call. |
| `width`   | Row length (pixels per row). |
| `height`  | Number of rows. |
| `simdFn`  | Called as `simdFn(size_t row, size_t colStart)` — handle the `Vec::Lanes` pixels starting at `(colStart, row)` together. |
| `scalarFn`| Called as `scalarFn(size_t row, size_t col)` — handle the 0–3 leftover pixels at the row end, one pixel per call. |
| `policy`  | Which `ExecutionPolicy` to run under. |

The `simdFn` receives a *starting* column, not a single index: it must
process the whole 4-pixel block. For an interleaved image, a helper may gather
four pixels into channel vectors and scatter them back; for best performance,
use contiguous channel planes so the loads and stores are naturally vector
friendly:

```cpp
const size_t w = src.width(), h = src.height();
parallelForRowsSimd<simd::Float4>(
    w, h,
    [&](size_t row, size_t colStart) {   // vector block: 4 pixels at once
        Pix4 p = loadRow4(src, (int32)colStart, (int32)row);   // 4 pixels in
        storeRow4(grayPix4(p), dst, (int32)colStart, (int32)row);   // 4 pixels out
    },
    [&](size_t row, size_t col) {        // tail: leftover 0-3 pixels
        pixel::writeRGBA(grayPixel(pixel::readPixel(src, (int32)col, (int32)row)),
                         dst, (int32)col, (int32)row);
    },
    policy);
```

### `parallelForColumnsSimd<Vec>(height, width, simdFn, scalarFn, policy)`

Same idea but the block is `Vec::Lanes` pixels **stacked vertically** in one
column. **Order is swapped: `height` first, then `width`.**

| Parameter | What it means |
|-----------|---------------|
| `Vec`     | SIMD container deciding block height. `simd::Float4` = 4 pixels per call. |
| `height`  | Column length (pixels per column) — **first** argument here. |
| `width`   | Number of columns — **second** argument here. |
| `simdFn`  | Called as `simdFn(size_t col, size_t rowStart)` — handle the 4 pixels from `(col, rowStart)` down to `(col, rowStart+3)`. |
| `scalarFn`| Called as `scalarFn(size_t col, size_t row)` — leftover rows at the column's end. |
| `policy`  | Which `ExecutionPolicy` to run under. |

```cpp
parallelForColumnsSimd<simd::Float4>(
    h, w,
    [&](size_t col, size_t rowStart) {   // vector block: 4 rows of this column
        Pix4 p = loadCol4(src, (int32)col, (int32)rowStart);
        storeCol4(invertPix4(p), dst, (int32)col, (int32)rowStart);
    },
    [&](size_t col, size_t row) {        // tail: leftover rows
        const Pixel p = pixel::readPixel(src, (int32)col, (int32)row);
        pixel::writeRGBA(invertOp(p), dst, (int32)col, (int32)row);
    },
    policy);
```

### `parallelForSimd<Vec>(n, simdFn, scalarFn, policy)` / `(begin, end, ...)`

The 1D twin of `parallelFor`, vectorized. Use it on a plain array of floats
(a look-up table, a per-pixel weight map, a float channel) where the data _is_
contiguous memory:

| Parameter | What it means |
|-----------|---------------|
| `Vec`     | SIMD container deciding block width. `simd::Float4` = 4 elements per call. |
| `n`       | Number of elements; shortcut for `(0, n)`. The `(begin, end)` overload works exactly like `parallelFor`'s. |
| `simdFn`  | Called as `simdFn(size_t blockStart)` — process the 4 elements `[blockStart, blockStart+4)`, e.g. with `simd::load` / `simd::store`. |
| `scalarFn`| Called as `scalarFn(size_t i)` — leftover tail elements. |
| `policy`  | Which `ExecutionPolicy` to run under. |

Real example — the grayscale filter, but computed on three contiguous float
planes so the luma math runs as real 4-lane arithmetic (3 loads + 1 store per
4 pixels):

```cpp
std::vector<float> red(n), green(n), blue(n), luma(n);   // n = w * h
for (size_t i = 0; i < n; ++i) {                         // build the planes once
    const Pixel p = pixel::readPixel(src, (int32)(i % w), (int32)(i / w));
    red[i] = p.r; green[i] = p.g; blue[i] = p.b;
}
parallelForSimd<simd::Float4>(
    n,
    [&](size_t start) {                          // 4 pixels' worth at once
        const simd::Float4 r = simd::load<float, 4>(&red[start]);
        const simd::Float4 g = simd::load<float, 4>(&green[start]);
        const simd::Float4 b = simd::load<float, 4>(&blue[start]);
        simd::store<float, 4>(&luma[start],
                              simd::clamp01(r * 0.2126f + g * 0.7152f + b * 0.0722f));
    },
    [&](size_t i) {                              // leftover tail values
        luma[i] = math::clamp01(red[i] * 0.2126f + green[i] * 0.7152f + blue[i] * 0.0722f);
    },
    policy);
```

---

## 3.5 Reading and writing images from parallel code

- **Reads are safe**: `ConstImageView` may be read from many threads. Pass
  `src.view()` (or `src.cview()`) freely.
- **Writes must be disjoint**: pit rows against columns, or index ranges
  against array slices. `parallelForRows` / `parallelForColumns` give that for
  free; for `parallelFor` make each index write only `out[i]`.
- The thread pool is a **process-wide shared resource**; keep jobs short and
  do not spawn jobs from inside a job.
- You almost never touch raw pixel buffers for this: `pixel::readPixel` /
  `pixel::writeRGBA` format-convert for you on 8-bit, 16-bit and float images.

## 3.6 Cheat sheet — which loop when

| You want to...                                | You write                                                       |
|-----------------------------------------------|-----------------------------------------------------------------|
| One scalar task per image row                 | `parallelForRows(h, fn, policy)`                                |
| One scalar task per image column              | `parallelForColumns(w, fn, policy)`                             |
| Loop over any 1D range (arrays, partial image)| `parallelFor(begin, end, fn, policy)` or `parallelFor(n, ...)`  |
| Vectorize a 1D float array                    | `parallelForSimd<Float4>(n, simdFn, scalarFn, policy)`          |
| Vectorize a whole image, row by row                 | `parallelForRowsSimd<Float4>(w, h, simdFn, scalarFn, policy)` |
| Vectorize a whole image, column by column     | `parallelForColumnsSimd<Float4>(h, w, simdFn, scalarFn, policy)`|
| Force single thread                            | `ExecutionPolicy::serial()`                                     |
| Force all cores and request SIMD               | `ExecutionPolicy::simdParallel()`                               |
| Let the library decide                         | `ExecutionPolicy()` (Auto) or `wantsParallel(policy, n, threshold)` |
| Parallel safety                                | every index writes only its own `out[i]`                        |

---

## 3.7 The most performant loops + benchmarking with `iml::bench`

`parallelForRowsSimd` with `simdParallel()` can combine the two big wins:
**all cores** and **4 values per vector instruction**. The benefit depends on
the data layout and the amount of work per pixel. To show that honestly, the
demo times it (and its siblings) with the library's own benchmark helper.
There is nothing special to install — one extra include:

```cpp
#include "imagelib/benchmark/Benchmark.h"   // iml::bench: everything below
```

### The freestanding helpers

```cpp
double ms = bench::bestMs([&]{ myFilter(src, dst, policy); });
// best of 5 measured runs, after 1 warm-up run (warm-up lets the thread pool
// and caches settle). bestMs(iterations, warmup, fn) and timeOnce(fn) also exist.

double mpix = bench::mpixPerSec((uint64)w * h, ms);   // Mega-pixels/second
```

The rules the helper enforces are the rules you should always follow when
timing anything:

1. **Warm up** before timing (thread pool + caches).
2. Take the **best** of several runs, not the first — it is the closest to the
   machine's real capability (least noise).
3. Report **ms and Mpix/s** for a known image size so results compare.

### The report class: `bench::BenchReport`

For a tidy table, put rows into a `BenchReport` and print them:

```cpp
const uint64 pixels = (uint64)src.width() * src.height();
bench::BenchReport rep;

rep.run("scalar serial",         pixels, 5, 2, [&]{ scaleKernel(...); });
rep.run("rows serial",           pixels, 5, 2, [&]{ rowKernel(..., ExecutionPolicy::serial()); });
rep.run("rows parallel",         pixels, 5, 2, [&]{ rowKernel(..., ExecutionPolicy::parallel()); });
rep.run("rows SIMD+parallel",    pixels, 5, 2, [&]{ rowKernel(..., ExecutionPolicy::simdParallel()); });

rep.print();      // columns: ms (best) | Mpix/s | vs first
```

- `run(name, pixels, iterations, warmup, fn)` — times `fn`, shows throughput.
  The `pixels` argument only feeds the Mpix/s column; pass the image size.
- `pair(name, pixels, iterations, warmup, serialFn, parallelFn)` — runs the
  **same** kernel under two policies, then prints a serial-vs-parallel table
  with a **speedup** column (`serialFn` is the baseline):

```cpp
bench::BenchReport rep;
rep.pair("gray rows", pixels, 5, 2,
         [&]{ grayRowsSimd(src, dst, ExecutionPolicy::simd()); },        // serial baseline
         [&]{ grayRowsSimd(src, dst, ExecutionPolicy::simdParallel()); }); // the fast one
rep.printPairs();
```

Other members you may meet: `add(name, pixels, ms, budgetMs)` (add a result
you already measured), `runBudget` / `pairBudget` (flag rows over a time
budget), `printEntry(i)` (single row). The whole tiny API lives in
`include/imagelib/benchmark/Benchmark.h`.

The demo's benchmark section measures scalar rows, serial SIMD rows, parallel
rows, and SIMD+parallel rows on the real photo. The SIMD row can be close to
or slower than scalar rows when format conversion dominates; the contiguous
plane example is the appropriate comparison for vector arithmetic itself.

### Reading the numbers you will see

Running the demo, the **rows serial** → **rows parallel** jump is often the big
one: that is the thread pool doing its job. SIMD rows can be roughly equal to
or slower than scalar rows when every pixel crosses a format-conversion API.
SIMD accelerates most clearly on contiguous float/byte buffers, such as the
plane loop in this tutorial and the `simd::averageU8`-style helpers.
When comparing, always report ms and Mpix/s for the same image so the rows
mean something.

Next: [Tutorial 4 — SIMD](04_simd.md) — what "SIMD" means under the hood and
the two high-level APIs (`simd::Vec`, the `*Simd` loops) you might actually
touch.