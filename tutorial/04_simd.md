# Tutorial 4 — SIMD Without the Pain

**Demo:** `04_simd.cpp` — it prints what your CPU can do, then shows the two
high-level SIMD APIs: `iml::simd::Vec` for "4 numbers at once" and
`parallelForRowsSimd` for whole images.

## 4.1 What "SIMD" actually means

Ordinary code does math on **one number at a time**. A modern CPU has wider
registers (128/256/512 bits) that can hold several numbers and do the same
operation on all of them **in one instruction**. That is *S*ingle
*I*nstruction, *M*ultiple *D*ata — SIMD.

Flying 4 floats at once can approach a 3.5–4x arithmetic speedup, but whole
image throughput also includes loads, stores, format conversion, and thread
coordination.
16 bytes at once (SSE2, baseline x86-64) ≈ the floor; 32 bytes (AVX2) or
64 bytes (AVX-512) with a native build.

For **contiguous pixel/channel data this is a good fit**. Interleaved RGB
storage is less ideal because each channel must be gathered and the result
must be packed back into the image format.

```cpp
// scalar thought:  out = in * 2 + 1   ... repeat for 4 pixels
// SIMD thought:    4 pixels in one register, one instruction, one store
```

## 4.2 You rarely write SIMD yourself

The library's execution paths can use SIMD when the kernel and layout support
it. You opt in by **choosing an `ExecutionPolicy`**:

```cpp
proc::mapPixels(src, dst, fn, ExecutionPolicy::simdParallel()); // request cores + SIMD
```

That is the main way a beginner "uses SIMD" in production: select the policy
and keep the data contiguous where possible. (The low-level intrinsics in
`include/imagelib/simd/Simd.h` exist for the library authors; you don't need
them.)

## 4.3 Knowing what you've got: the info functions

```cpp
simd::simdAvailable()      // does this CPU have any vector ISA?
simd::activeIsaName()      // "SSE2", "AVX2", ... a string
simd::vectorBytes()        // 16/32/64 bytes per vector op
simd::simdWidth<float>()   // how many floats you can do at once
simd::assuredFeatures()    // features compiled-in AND present on this CPU
```

`assuredFeatures()` is the honest one — the ISA your binary is safe to
execute, which is what the library actually dispatches on. On a stock build
x86-64 that is always at least SSE2 (16 bytes → 4 floats or 16 bytes-at-once).

## 4.4 The small API: `simd::Vec` — 4 numbers as one value

For the rare loop you hand-vectorize, ImageLib's `simd::SimdVec<float,4>`
(alias `Float4`) behaves like a tiny vector with normal arithmetic:

```cpp
simd::Float4 a(1.f, .5f, 0.f, -.25f);
simd::Float4 v = a * 2.f + 0.25f;      // every lane: a*2 + 0.25  (one step)
v = simd::clamp01(v);                  // keep each lane in [0,1]
simd::store(dst, v);                   // write all 4 lanes back to memory
```

- `simd::load(ptr)` / `simd::store(ptr, v)` — move 4 values between memory
  and the register without alignment headaches.
- lanes are `x, y, z, w` or `v[0..3]`.
- `simd::sum(v)`, `simd::dot(a,b)`, `simd::min(a,b)`, `simd::max(a,b)`,
  `simd::abs(v)`, `simd::clamp(v, lo, hi)` cover the common math.

A couple of ready-made byte helpers already pick the fastest ISA for you:

```cpp
simd::averageU8(dst, a, b, n);        // (a[i] + b[i] + 1) / 2
simd::saturatingAddU8(dst, a, b, n);  // min(a[i] + b[i], 255)
```

Always check the width: `simd::simdWidth<float>()` tells you how many floats
fit in a vector; if the loop length isn't a multiple of that, the leftover
"tail" needs scalar handling (below). A four-lane operation is not
automatically a four-times-faster image filter: interleaved RGB loads and
stores can dominate the arithmetic.

## 4.5 The loop API: `parallelForSimd` / `parallelForRowsSimd`

Instead of writing head+vector+tail yourself, the library does the split and
even parallelizes it. Here is the grayscale filter from Tutorial 1 running on
SIMD: three contiguous float planes (red/green/blue) get reduced to a luma
plane 4 values at a time:

```cpp
parallelForRowsSimd<simd::Float4>(
    width, height,
    [&](size_t row, size_t colStart) {          // SIMD: one 4-pixel block
        const size_t off = row * width + colStart;
        auto r = simd::load<float, 4>(&red[off]);   // 4 reds   (one instruction)
        auto g = simd::load<float, 4>(&green[off]); // 4 greens
        auto b = simd::load<float, 4>(&blue[off]);  // 4 blues
        simd::store<float, 4>(&luma[off],
                              simd::clamp01(r * 0.2126f + g * 0.7152f + b * 0.0722f));
    },
    [&](size_t row, size_t col) {               // scalar: leftover pixels
        const size_t off = row * width + col;
        luma[off] = iml::math::clamp01(red[off] * 0.2126f +
                                       green[off] * 0.7152f + blue[off] * 0.0722f);
    },
    ExecutionPolicy::simdParallel());           // request all cores + vectors
```

You provide two lambdas — the vector one (gets a block start index) and the
scalar one (gets a single index for the 0–3 leftover pixels per row). The
library loops the vector lambda over blocks, loops the scalar lambda over the
tail, and runs the whole thing over the thread pool when the policy asks.
For `parallelForRowsSimd` and `parallelForColumnsSimd`, automatic parallel
selection considers the total pixels, not merely the number of rows or vector
blocks. An explicit `grainSize` can control task sizing.

`parallelForSimd<Vec>(n, simdFn, scalarFn, policy)` is the same idea for a
plain 1D range.

## 4.6 Summary

| You want to...                        | You write                                                     |
|---------------------------------------|---------------------------------------------------------------|
| Turn on SIMD without code             | pass `ExecutionPolicy::simdParallel()`                        |
| Learn what your CPU can do            | `simd::activeIsaName()`, `simd::simdWidth<float>()`           |
| Process 4 (or N) values as one        | `simd::Float4`, `simd::load`, `simd::store`, arithmetic ops   |
| Speed up a per-row filter             | `parallelForRowsSimd<Vec>(w, h, simdFn, scalarFn, policy)`    |
| Sort out leftover elements            | the `scalarFn` tail lambda — the library calls it for you     |
| Avoid RGB gather overhead             | use contiguous channel planes or a format-specific packed kernel |

You are done with the tour.