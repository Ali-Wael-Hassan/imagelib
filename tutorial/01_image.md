# Tutorial 1 — Working with Images

**Demo:** `01_image.cpp` — run it and it loads a picture, converts it to
grayscale, inverts it, and saves everything to disk. It writes the grayscale
and invert kernels **by hand** — the library never does those for you here —
because that is exactly what the assignment asks you to write.

You do not need to know how PNG/JPEG/BMP are encoded, how memory is managed,
or what a codec is. In this library that is all hidden behind one object:
`iml::Image`.

## 1.1 The one class you use all day: `Image`

An `Image` **owns** pixel memory. It loads files, it knows its size and
format, it hands you pixels, and it saves itself.

```cpp
#include "imagelib/imagelib.h"
using namespace iml;

Image photo("assets/mario.bmp");          // load + decode in one line

int w = photo.width();                    // width in pixels
int h = photo.height();                   // height in pixels
int ch = photo.channels();                // 1 (gray) / 3 (RGB) / 4 (RGBA)

photo.save("copy.png");                   // the extension picks the codec
```

Two things to notice because they are true for all your C++ programs:

- `Image` is a **value type that deep-copies**. `Image a = photo;` copies
  *all* the pixels. That is exactly what you want here (images are small and
  copying is safe), and it is the main difference from the `ImageView` below.
- The copy only happens on copy. Moving (`Image a = std::move(photo)`)
  transfers the pixel memory without copying, which is why returning an
  `Image` from a function is cheap.

If a file is missing or corrupt, the constructor *throws* an `iml::Error`.
Production code wraps risky calls in `try/catch`:

```cpp
try {
    Image img("assets/mario.bmp");
    img.save("out.png");
} catch (const iml::Error& e) {
    std::cerr << "Failed: " << e.what() << '\n';
}
```

## 1.2 Views: looking at pixels without owning them

`Image::view()` returns an `ImageView` — a cheap, non-owning "window" over
the pixels. You pass *views* around, not images:

```cpp
ImageView        view = img.view();      // read + write
ConstImageView cview = img.view();       // read-only (also written view())
```

The `const` part is your safety net. Filter functions take
`const ConstImageView& src` (I promise not to touch your picture) and
`ImageView dst` (I will write into this one). If the source is passed by
const reference, the compiler *forces* the read-only view.

## 1.3 Pixels are floats in [0, 1]

Whatever the storage format (`UInt8`, `UInt16`, `Float32`), the library hands
you a `Pixel` whose channels are **normalized floats** — 0.0 is black, 1.0 is
full. One `Pixel` is always RGBA:

```cpp
Pixel p = img.readPixel(x, y);        // read one pixel
float r = p.r, g = p.g, b = p.b, a = p.a;

img.write(Pixel(1.f, 0.f, 0.f), x, y);  // write a red pixel
```

Because you never touch `255`, the same code works on 8-bit and 16-bit
images. The low-level helpers behind this are `iml::pixel::readPixel` /
`iml::pixel::writeRGBA`; you will meet them in the filter templates.

## 1.4 Filters are yours to write — the library is the tool

Two operations cover the pattern every filter in the assignment follows:
**read any pixel**, **do your math**, **write any pixel**. That is all there is.

**Grayscale** (render one channel from the luma of three):

```cpp
Image gray(fmt::gray8, img.width(), img.height(), ColorSpace::Gray);
for (int32 y = 0; y < (int32)img.height(); ++y)
    for (int32 x = 0; x < (int32)img.width(); ++x) {
        const Pixel p = pixel::readPixel(img.view(), x, y);
        const float l = math::clamp01(0.2126f * p.r + 0.7152f * p.g + 0.0722f * p.b); // Rec.709
        pixel::writeNorm(gray.view(), x, y, 0, l);   // gray images have one channel
    }
```

**Invert** (mirror every channel around 0.5):

```cpp
for (int32 y = 0; y < (int32)img.height(); ++y)
    for (int32 x = 0; x < (int32)img.width(); ++x) {
        const Pixel p = pixel::readPixel(img.view(), x, y);
        pixel::writeRGBA(Pixel(1.f - p.r, 1.f - p.g, 1.f - p.b, p.a), img.view(), x, y);
    }
```

The tools doing the work (`iml::pixel::readPixel` / `writeRGBA` / `writeNorm`)
convert between the on-disk format and a normalized `float` `Pixel` for you,
so both of these kernels work unchanged on 8-bit, 16-bit and float images.

That is the whole trick of the assignment's filters: they are this same loop
with different math in the middle. Later lessons show how to run the loop on
multiple cores (Tutorial 3) and on SIMD registers (Tutorial 4). The math and
output stay the same, but the speedup depends on the data layout: contiguous
planes vectorize especially well, while interleaved RGB pixels still require
channel gathering and packing.

## 1.5 Creating new images

`Image(format, w, h, colorSpace)` allocates a fresh image:

```cpp
Image out(fmt::rgb8, 640, 480, ColorSpace::SRGB);
```

`colorSpace` matters for interpreting values (`SRGB` for photos, `Gray` for
grayscale), and the `fmt::` presets let you say `gray8`, `rgb8`, `rgba8`,
`rgba32f`, ... without remembering struct fields.

`Image::clone()` deep-copies an existing image — handy when you want a
scratch copy to draw on without changing the original.

## 1.6 Summary

| You want to...          | You write                                      |
|-------------------------|------------------------------------------------|
| Load a file             | `Image img(path);`                             |
| Save a file             | `img.save("out.png");`                         |
| Read metadata           | `img.width()`, `img.height()`, `img.channels()` |
| Read one pixel          | `pixel::readPixel(view, x, y)` → `Pixel` floats [0,1] |
| Write one pixel         | `pixel::writeRGBA(Pixel(...), view, x, y)`     |
| Look without owning     | `img.view()` → `ImageView` / `img.cview()`     |
| Write a filter          | read a pixel -> do your math -> write it back (`grayManual`, `invertManual`) |
| Deal with errors        | `try { ... } catch (const iml::Error& e) {...}`|

Next: [Tutorial 2 — Math](02_math.md) — the little vector math you need to
make pixels *move* (drawing, shading, coordinates).