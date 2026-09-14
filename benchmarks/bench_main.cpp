// benchmarks/bench_main.cpp
//
// ImageLib kernel benchmarks. Every kernel is measured twice — single-threaded
// scalar (serial) and the optimized path (SIMD + thread pool) — so each row
// pair shows the faster/multithreaded speedup directly in the "vs first"
// column. These timings are the numbers students should recreate with their
// own kernels (see examples/custom/).
//
// Output is an aligned table; run with `imagelib_bench [runs] [warmup]`.
#include "imagelib/imagelib.h"
#include "imagelib/benchmark/Benchmark.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>

using namespace iml;
using namespace iml::bench;
using namespace iml::proc;

namespace {

const uint32 W = 512, H = 512;
const uint64 PIX = static_cast<uint64>(W) * H;
const ExecutionPolicy ser = ExecutionPolicy::serial();
const ExecutionPolicy par = ExecutionPolicy::simdParallel();

Image g_makeGray() { return Image(fmt::gray32f, W, H, ColorSpace::Gray); }
Image src  = g_makeGray();
Image dst  = g_makeGray();
Image big (fmt::gray32f, 1024, 1024, ColorSpace::Gray);
Image pic (fmt::rgb8, W, H, ColorSpace::SRGB);
Image pout(fmt::rgb8, W, H, ColorSpace::SRGB);

void benchConvolve(const ExecutionPolicy& p) {
    conv::convolve(src.view(), dst.view(), conv::sobelXKernel(), conv::BorderMode::Clamp, p);
}
void benchGaussian(const ExecutionPolicy& p) {
    conv::gaussianBlur(src.view(), dst.view(), 1.0f, conv::BorderMode::Clamp, p);
}
void benchResizeUp(const ExecutionPolicy& p) {
    resize::resize(src.view(), big.view(), resize::Filter::Bilinear, conv::BorderMode::Clamp, p);
}
void benchResizeDown(const ExecutionPolicy& p) {
    resize::resize(big.view(), src.view(), resize::Filter::Bilinear, conv::BorderMode::Clamp, p);
}
void benchNoise(const ExecutionPolicy& p) {
    procgen::NoiseSettings s;
    s.type = procgen::NoiseType::Perlin;
    s.seed = 42;
    s.frequency = 4.f;
    procgen::fillNoise(dst.view(), s, p);
}
void benchQuantize(const ExecutionPolicy& p) {
    comp::QuantizeParams q;
    q.bitsR = 5; q.bitsG = 6; q.bitsB = 5; q.dither = comp::DitherMode::Ordered4x4;
    comp::quantizeImage(pic.view(), pout.view(), q, p);
}
void benchToGray(const ExecutionPolicy& p) {
    color::toGray(pic.view(), pout.view(), p);
}
void benchInvert(const ExecutionPolicy& p) {
    filter::invert(pic.view(), pout.view(), p);
}
void benchThreadPool() {
    ThreadPool pool(4);
    std::atomic<long long> sum{0};
    for (int i = 0; i < 10000; ++i)
        pool.push(Job([&sum]() { sum.fetch_add(1, std::memory_order_relaxed); }));
    pool.waitAll();
    if (sum.load() != 10000) std::abort();
}

/// Measures a kernel with both policies and stores it as one pair row.
template <class Fn>
void benchPair(BenchReport& rep, const char* name, Fn&& fn, uint64 pixels,
               int runs, int warmup) {
    rep.pair(name, pixels, runs, warmup,
             [&]{ fn(ser); },
             [&]{ fn(par); });
}

} // namespace

int main(int argc, char** argv) {
    int runs   = argc > 1 ? std::atoi(argv[1]) : 5;
    int warmup = argc > 2 ? std::atoi(argv[2]) : 2;
    if (runs < 1) runs = 1;
    if (warmup < 0) warmup = 0;

    bench::fillGradient(src);
    bench::fillGradient(pic);

    std::printf("ImageLib benchmarks (best of %d, %d warm-ups, %dx%d frames)\n",
                runs, warmup, W, H);
    std::printf("active ISA: %s, workers: %zu\n", simd::activeIsaName(),
                defaultThreadPool().workerCount());

    const size_t bigPix = 1024ull * 1024;
    BenchReport rep;
    benchPair(rep, "sobel 3x3", benchConvolve, PIX, runs, warmup);
    benchPair(rep, "gaussian blur sigma 1", benchGaussian, PIX, runs, warmup);
    benchPair(rep, "bilinear resize up 512->1024", benchResizeUp, bigPix, runs, warmup);
    benchPair(rep, "bilinear resize down 1024->512", benchResizeDown, PIX, runs, warmup);
    benchPair(rep, "perlin noise fill", benchNoise, PIX, runs, warmup);
    benchPair(rep, "quantize RGB8 (ordered 4x4)", benchQuantize, PIX, runs, warmup);
    benchPair(rep, "toGray", benchToGray, PIX, runs, warmup);
    benchPair(rep, "invert", benchInvert, PIX, runs, warmup);
    rep.printPairs();

    // Thread pool throughput (jobs per second-ish; no pixel count).
    // Align the label with the report's dynamically sized name column.
    size_t nameW = std::string("benchmark").size();
    for (size_t i = 0; i < rep.pairSize(); ++i)
        nameW = std::max(nameW, rep.pairLabel(i).size());
    double ms = bestMs(runs, warmup, benchThreadPool);
    std::printf("%-*s %12.4f ms   thread pool, 10000 jobs / 4 workers\n",
                static_cast<int>(nameW), "thread pool push+wait", ms);

    return 0;
}