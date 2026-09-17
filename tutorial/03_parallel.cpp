// ImageLib Tutorial 03 - The complete parallelFor family, on a real image
// ------------------------------------------------------------------------
// Every parallel loop in iml::threading, with the exact parameters each one
// takes, applied to a real photo from assets/ (not a synthetic gradient):
//
//   parallelFor(n, fn, policy)                 invert the whole photo
//   parallelFor(begin, end, fn, policy)        grayscale ONLY the left half
//   parallelForRows(height, fn, policy)        grayscale, one task per row
//   parallelForColumns(width, fn, policy)      invert, one task per col
//   parallelForSimd<Float4>(n, simdFn, scalarFn, policy)   grayscale luma
//   parallelForRowsSimd<Float4>(width, height, simdFn, scalarFn, policy)
//                                                        grayscale, 4 px/block
//   parallelForColumnsSimd<Float4>(height, width, simdFn, scalarFn, policy)
//                                                        invert, 4 rows/block
//
// The example filters are grayscale and invert, written by hand (Tutorial 1):
// each lesson here is about the LOOP, not the filter. The last three lines
// batch 4 pixels per vector instruction; together with the thread pool they
// are the fast options. Part 2 benchmarks them with the library's own utility:
// iml::bench::bestMs / BenchReport::run / ::pair.
//
// Build + run (from the repository root):
//   g++ -std=c++17 -O2 -Iinclude -Isrc tutorial/03_parallel.cpp -Lbin -limagelib -lstb_image -o bin/tutorial_03_parallel.exe
//   ./bin/tutorial_03_parallel.exe [input_image] [output_folder]
//
// Examples:
//   ./bin/tutorial_03_parallel.exe                     (uses assets/mario.jpg)
//   ./bin/tutorial_03_parallel.exe assets/toy1.jpg tmp
//
// Note: Single-argument SimdVec construction is illegal; scale with a scalar
// (v * 0.2126f) instead of writing V4(0.2126f).

#include "imagelib/imagelib.h"
#include "imagelib/benchmark/Benchmark.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace iml;

namespace {

// ---------------------------------------------------------------------------
// Small IO helpers so the demo works from any working directory.
// ---------------------------------------------------------------------------

bool fileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

std::string pickInput(const std::string& given) {
    if (!given.empty())
        return given;
    const char* candidates[] = {
        "assets/mario.jpg",
        "assets/toy2.jpg",
        "assets/building.jpg",
        "assets/toy1.jpg",
        "assets/mario.bmp",
        "../assets/mario.jpg",
        "../../assets/mario.jpg",
    };
    for (const char* c : candidates)
        if (fileExists(c))
            return c;
    return "assets/mario.bmp";
}

std::string outPath(const std::string& folder, const std::string& file) {
    return folder.empty() ? file : folder + "/" + file;
}

Image formatLike(const ConstImageView& src) {
    return Image(src.format(), src.width(), src.height(), src.colorSpace(), src.alphaMode());
}

Image grayLike(const ConstImageView& src) {
    return Image(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);
}

// Shrinks a big photo (e.g. 5122 x 3227) to a working width so the benchmark
// below finishes in a couple of seconds. Runs once, before any timing.
Image fitWidth(const ConstImageView& src, uint32 maxW) {
    const uint32 w = std::min(src.width(), maxW);
    const uint32 h = (w >= src.width()) ? src.height()
                                        : std::max(1u, (uint32)((uint64)src.height() * w / src.width()));
    Image out(src.format(), w, h, src.colorSpace(), src.alphaMode());
    ImageView dv = out.view();
    for (uint32 y = 0; y < h; ++y) {
        const uint32 sy = (w >= src.width()) ? y : (uint32)((uint64)y * src.height() / h);
        for (uint32 x = 0; x < w; ++x) {
            const uint32 sx = (w >= src.width()) ? x : (uint32)((uint64)x * src.width() / w);
            pixel::writeRGBA(pixel::readPixel(src, (int32)sx, (int32)sy), dv, (int32)x, (int32)y);
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// The per-pixel kernels (scalar + SIMD twins for the same math).
// These are the hand-written gray + invert from Tutorial 1 - the only filters
// this lesson needs, so every loop below can focus on the loop itself.
// ---------------------------------------------------------------------------

Pixel grayPixel(const Pixel& p) {
    const float l = math::clamp01(0.2126f * p.r + 0.7152f * p.g + 0.0722f * p.b);
    return Pixel(l, l, l, 1.f);
}

Pixel invertOp(const Pixel& p) {
    return Pixel(1.f - p.r, 1.f - p.g, 1.f - p.b, p.a);
}

// ---------------------------------------------------------------------------
// 4 pixels packed into SIMD registers (gather/scatter via the pixel API).
// Most tutorials tell you to reach for raw buffer pointers; you don't need to.
// ---------------------------------------------------------------------------

using V4 = simd::Float4;

V4 splat(float v) { return simd::broadcast<float, 4>(v); }

struct Pix4 {
    V4 r, g, b, a;   // lane i holds the channel of pixel (x0+i, y)
};

Pix4 loadRow4(const ConstImageView& src, int32 x0, int32 y) {
    float r[4], g[4], b[4], a[4];
    for (int i = 0; i < 4; ++i) {
        const Pixel p = pixel::readPixel(src, x0 + i, y);
        r[i] = p.r; g[i] = p.g; b[i] = p.b; a[i] = p.a;
    }
    return {simd::load<float, 4>(r), simd::load<float, 4>(g),
            simd::load<float, 4>(b), simd::load<float, 4>(a)};
}

void storeRow4(const Pix4& p, ImageView dst, int32 x0, int32 y) {
    float r[4], g[4], b[4], a[4];
    simd::store<float, 4>(r, p.r);
    simd::store<float, 4>(g, p.g);
    simd::store<float, 4>(b, p.b);
    simd::store<float, 4>(a, p.a);
    for (int i = 0; i < 4; ++i)
        pixel::writeRGBA(Pixel(r[i], g[i], b[i], a[i]), dst, x0 + i, y);
}

Pix4 loadCol4(const ConstImageView& src, int32 x, int32 y0) {
    float r[4], g[4], b[4], a[4];
    for (int i = 0; i < 4; ++i) {
        const Pixel p = pixel::readPixel(src, x, y0 + i);
        r[i] = p.r; g[i] = p.g; b[i] = p.b; a[i] = p.a;
    }
    return {simd::load<float, 4>(r), simd::load<float, 4>(g),
            simd::load<float, 4>(b), simd::load<float, 4>(a)};
}

void storeCol4(const Pix4& p, ImageView dst, int32 x, int32 y0) {
    float r[4], g[4], b[4], a[4];
    simd::store<float, 4>(r, p.r);
    simd::store<float, 4>(g, p.g);
    simd::store<float, 4>(b, p.b);
    simd::store<float, 4>(a, p.a);
    for (int i = 0; i < 4; ++i)
        pixel::writeRGBA(Pixel(r[i], g[i], b[i], a[i]), dst, x, y0 + i);
}

// SIMD twins of the scalar kernels: the same math on 4 pixels at once.
Pix4 grayPix4(const Pix4& p) {
    const V4 l = simd::clamp01(p.r * 0.2126f + p.g * 0.7152f + p.b * 0.0722f);
    return {l, l, l, splat(1.f)};
}

Pix4 invertPix4(const Pix4& p) {
    return {splat(1.f) - p.r, splat(1.f) - p.g, splat(1.f) - p.b, p.a};
}

// ---------------------------------------------------------------------------
// The six parallelFor functions, one filter each.
// ---------------------------------------------------------------------------

// (1) parallelFor(n, fn, policy): invert the whole photo by linear index.
template <class Fn>
void linearMap(const ConstImageView& src, ImageView dst, size_t begin, size_t end, Fn fn,
               const ExecutionPolicy& policy) {
    const size_t w = src.width();
    parallelFor(
        begin, end,                                           // [begin, end)
        [&](size_t i) {
            const int32 x = (int32)(i % w);                   // index -> (x, y)
            const int32 y = (int32)(i / w);
            pixel::writeRGBA(fn(pixel::readPixel(src, x, y)), dst, x, y);
        },
        policy);
}

// (2) parallelForRows(height, fn, policy): one grayscale task per row.
void grayRows(const ConstImageView& src, ImageView dst, const ExecutionPolicy& policy) {
    const int32 w = (int32)src.width();
    parallelForRows(
        (size_t)src.height(),                                 // fn called once per row
        [&](size_t row) {
            const int32 y = (int32)row;
            for (int32 x = 0; x < w; ++x)
                pixel::writeRGBA(grayPixel(pixel::readPixel(src, x, y)), dst, x, y);
        },
        policy);
}

// (3) parallelForColumns(width, fn, policy): one invert task per column.
void invertColumns(const ConstImageView& src, ImageView dst, const ExecutionPolicy& policy) {
    parallelForColumns(
        (size_t)src.width(),                                  // fn called once per col
        [&](size_t col) {
            for (int32 y = 0; y < (int32)src.height(); ++y) {
                const Pixel p = pixel::readPixel(src, (int32)col, y);
                pixel::writeRGBA(invertOp(p), dst, (int32)col, y);
            }
        },
        policy);
}

// (4) parallelForSimd<Float4>(n, simdFn, scalarFn, policy): the same grayscale
// filter, but on contiguous float planes so the luma math really vectorizes
// (3 loads + 1 store per 4 floats).
void grayPlanesSimd(const ConstImageView& src, ImageView dst, const ExecutionPolicy& policy) {
    const size_t w = src.width(), h = src.height(), n = w * h;
    std::vector<float> red(n), green(n), blue(n), luma(n);    // one float per pixel
    for (size_t i = 0; i < n; ++i) {
        const int32 x = (int32)(i % w), y = (int32)(i / w);
        const Pixel p = pixel::readPixel(src, x, y);
        red[i] = p.r;
        green[i] = p.g;
        blue[i] = p.b;
    }
    parallelForSimd<simd::Float4>(
        n,                                                    // [0, n)
        [&](size_t start) {                                   // vector block start
            const simd::Float4 r = simd::load<float, 4>(&red[start]);
            const simd::Float4 g = simd::load<float, 4>(&green[start]);
            const simd::Float4 b = simd::load<float, 4>(&blue[start]);
            simd::store<float, 4>(&luma[start],
                                  simd::clamp01(r * 0.2126f + g * 0.7152f + b * 0.0722f));
        },
        [&](size_t i) {                                       // leftover tail
            luma[i] = math::clamp01(red[i] * 0.2126f + green[i] * 0.7152f + blue[i] * 0.0722f);
        },
        policy);
    for (size_t i = 0; i < n; ++i)
        pixel::writeNorm(dst, (int32)(i % w), (int32)(i / w), 0, luma[i]);
}

// (5) parallelForRowsSimd<Float4>(width, height, simdFn, scalarFn, policy):
// grayscale with 4 pixels per vector block. width FIRST, then height.
void grayRowsSimd(const ConstImageView& src, ImageView dst, const ExecutionPolicy& policy) {
    const size_t w = src.width(), h = src.height();
    parallelForRowsSimd<simd::Float4>(
        w, h,
        [&](size_t row, size_t colStart) {                    // vector block
            Pix4 p = loadRow4(src, (int32)colStart, (int32)row);   // 4 pixels in
            storeRow4(grayPix4(p), dst, (int32)colStart, (int32)row);  // 4 out
        },
        [&](size_t row, size_t col) {                         // leftover tail pixel
            pixel::writeRGBA(grayPixel(pixel::readPixel(src, (int32)col, (int32)row)),
                             dst, (int32)col, (int32)row);
        },
        policy);
}

// (6) parallelForColumnsSimd<Float4>(height, width, simdFn, scalarFn, policy):
// invert with 4 pixels stacked vertically per block. height FIRST.
void invertColumnsSimd(const ConstImageView& src, ImageView dst, const ExecutionPolicy& policy) {
    const size_t w = src.width(), h = src.height();
    parallelForColumnsSimd<simd::Float4>(
        h, w,
        [&](size_t col, size_t rowStart) {                    // vector block
            Pix4 p = loadCol4(src, (int32)col, (int32)rowStart);   // 4 pixels in
            storeCol4(invertPix4(p), dst, (int32)col, (int32)rowStart);  // 4 out
        },
        [&](size_t col, size_t row) {                         // leftover tail pixel
            const Pixel p = pixel::readPixel(src, (int32)col, (int32)row);
            pixel::writeRGBA(invertOp(p), dst, (int32)col, (int32)row);
        },
        policy);
}

// ---------------------------------------------------------------------------
// Benchmarks with iml::bench.
// ---------------------------------------------------------------------------

void grayScalar(const ConstImageView& src, ImageView dst) {
    for (int32 y = 0; y < (int32)src.height(); ++y)
        for (int32 x = 0; x < (int32)src.width(); ++x)
            pixel::writeRGBA(grayPixel(pixel::readPixel(src, x, y)), dst, x, y);
}

void benchmarkGray(const ConstImageView& src) {
    const uint64 pixels = (uint64)src.width() * src.height();
    Image dst = grayLike(src);

    // One throwaway parallel run wakes the thread pool before timing starts.
    grayRows(src, dst.view(), ExecutionPolicy::parallel());

    std::cout << "--- direct helpers: bench::bestMs + bench::mpixPerSec ---\n";
    const double msScalar = bench::bestMs([&] { grayScalar(src, dst.view()); });
    const double msFast = bench::bestMs([&] { grayRowsSimd(src, dst.view(), ExecutionPolicy::simdParallel()); });
    std::cout << "scalar serial     : " << msScalar << " ms  "
              << bench::mpixPerSec(pixels, msScalar) << " Mpix/s\n";
    std::cout << "rows SIMD+parallel: " << msFast << " ms  "
              << bench::mpixPerSec(pixels, msFast) << " Mpix/s ("
              << msScalar / msFast << "x)\n\n";

    std::cout << "--- bench::BenchReport::run -> rep.print() ---\n";
    bench::BenchReport rep;
    rep.run("scalar serial", pixels, 5, 2, [&] { grayScalar(src, dst.view()); });
    rep.run("rows serial", pixels, 5, 2, [&] { grayRows(src, dst.view(), ExecutionPolicy::serial()); });
    rep.run("rows parallel", pixels, 5, 2, [&] { grayRows(src, dst.view(), ExecutionPolicy::parallel()); });
    rep.run("rows SIMD+parallel", pixels, 5, 2, [&] { grayRowsSimd(src, dst.view(), ExecutionPolicy::simdParallel()); });
    rep.print();

    std::cout << "\n--- bench::BenchReport::pair -> rep.printPairs() ---\n"
              << "same kernel, serial SIMD vs SIMD + all cores:\n";
    bench::BenchReport pairs;
    pairs.pair(
        "gray rows (SIMD)", pixels, 5, 2,
        [&] { grayRowsSimd(src, dst.view(), ExecutionPolicy::simd()); },
        [&] { grayRowsSimd(src, dst.view(), ExecutionPolicy::simdParallel()); });
    pairs.printPairs();
}

} // namespace

int main(int argc, char** argv) {
    const std::string input = pickInput(argc > 1 ? argv[1] : "");
    const std::string outDir = argc > 2 ? argv[2] : "";

    try {
        // ---- a REAL photo, not a generated gradient -------------------------
        const Image photo(input);
        std::cout << "Loaded real image: " << input << '\n';
        std::cout << "Photo size       : " << photo.width() << " x " << photo.height()
                  << ", " << photo.channels() << " channel(s)\n";

        // Shrink to a working width so the benchmark finishes in seconds.
        const Image work = fitWidth(photo.cview(), 1024u);
        const size_t n = (size_t)work.width() * work.height();
        std::cout << "Working size     : " << work.width() << " x " << work.height()
                  << " (" << n << " pixels)\n";

        std::cout << "\n=== PART 1: all six parallelFor functions, on this image ===\n";

        // (1) parallelFor(n, fn, policy) - invert by linear pixel index.
        Image inv = formatLike(work.view());
        linearMap(work.cview(), inv.view(), 0, n, invertOp, ExecutionPolicy::parallel());
        inv.save(outPath(outDir, "03_invert.png"));
        std::cout << "parallelFor(n)            -> 03_invert.png\n";

        // (1b) parallelFor(begin, end, fn, policy) - grayscale only the left half.
        Image half = work.clone();
        linearMap(work.cview(), half.view(), 0, n / 2, grayPixel, ExecutionPolicy::parallel());
        half.save(outPath(outDir, "03_gray_half.png"));
        std::cout << "parallelFor(begin,end)     -> 03_gray_half.png\n";

        // (2) parallelForRows(height, fn, policy) - grayscale, one task per row.
        Image grayR = grayLike(work.view());
        grayRows(work.cview(), grayR.view(), ExecutionPolicy::parallel());
        grayR.save(outPath(outDir, "03_gray_rows.png"));
        std::cout << "parallelForRows            -> 03_gray_rows.png\n";

        // (3) parallelForColumns(width, fn, policy) - invert, one per column.
        Image invC = formatLike(work.view());
        invertColumns(work.cview(), invC.view(), ExecutionPolicy::parallel());
        invC.save(outPath(outDir, "03_invert_columns.png"));
        std::cout << "parallelForColumns         -> 03_invert_columns.png\n";

        // (4) parallelForSimd<Float4>(n, simdFn, scalarFn, policy) - gray luma.
        Image grayP = grayLike(work.view());
        grayPlanesSimd(work.cview(), grayP.view(), ExecutionPolicy::simdParallel());
        grayP.save(outPath(outDir, "03_gray_planes_simd.png"));
        std::cout << "parallelForSimd            -> 03_gray_planes_simd.png\n";

        // (5) parallelForRowsSimd<Float4>(width, height, ...) - gray, 4 px/block.
        Image grayS = grayLike(work.view());
        grayRowsSimd(work.cview(), grayS.view(), ExecutionPolicy::simdParallel());
        grayS.save(outPath(outDir, "03_gray_rows_simd.png"));
        std::cout << "parallelForRowsSimd        -> 03_gray_rows_simd.png\n";

        // (6) parallelForColumnsSimd<Float4>(height, width, ...) - 4 rows/block.
        Image invS = formatLike(work.view());
        invertColumnsSimd(work.cview(), invS.view(), ExecutionPolicy::simdParallel());
        invS.save(outPath(outDir, "03_invert_columns_simd.png"));
        std::cout << "parallelForColumnsSimd     -> 03_invert_columns_simd.png\n";

        std::cout << "\n=== PART 2: benchmarking the fastest loops (iml::bench) ===\n";
        benchmarkGray(work.cview());

        std::cout << "\nOpen the saved PNGs to see each loop's output.\n";
        return 0;
    } catch (const Error& e) {
        std::cerr << "ImageLib error: " << e.what() << '\n';
        return 1;
    }
}