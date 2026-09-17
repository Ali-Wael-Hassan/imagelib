#pragma once
/// @file examples/example_util.h
/// Shared plumbing for the optimized filter demos:
///   * input/output path selection and one-word CLI parsing
///   * a 4-pixel SIMD block type (Pix4) over iml::simd::Float4
///   * the per-row SIMD mapper used by the point filters
///   * tiny report printer built on iml::bench

#include "imagelib/imagelib.h"
#include "imagelib/benchmark/Benchmark.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace iml; // ImageLib lives in the iml:: namespace.

namespace iml_example {

// ---------------------------------------------------------------------------
// Paths / arguments
// ---------------------------------------------------------------------------

/// Returns true when `path` exists on disk.
inline bool fileExists(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f)
        return false;
    std::fclose(f);
    return true;
}

/// Picks a demo image: the user path when given, otherwise the first asset
/// that exists relative to the working directory.
inline std::string pickInput(const std::string& given) {
    if (!given.empty())
        return given;
    const char* candidates[] = {
        "assets/mario.bmp", "assets/photographer.bmp", "assets/toy1.jpg",
        "../assets/mario.bmp", "../../assets/mario.bmp",
    };
    for (const char* c : candidates)
        if (fileExists(c))
            return c;
    return given;
}

/// Parsed command line shared by every demo:
///   program [input] [output] ["serial"] [param ...]
struct RunArgs {
    /// Input image path ("" lets pickInput() choose an asset).
    std::string input;
    /// Output image path; "" means "don't save".
    std::string output;
    /// Per-filter parameters in order (scale, angle, radius, second image, ...).
    std::vector<std::string> params;
    /// When true run the naive scalar kernel only (no report).
    bool serial = false;
};

inline RunArgs parse(int argc, char** argv, const char* defaultOut) {
    RunArgs a;
    a.input = pickInput(argc > 1 ? argv[1] : "");
    a.output = argc > 2 ? argv[2] : defaultOut;
    for (int i = 3; i < argc; ++i) {
        if (std::string(argv[i]) == "serial")
            a.serial = true;
        else
            a.params.emplace_back(argv[i]);
    }
    return a;
}

/// Returns params[i] or `def` when missing.
inline const std::string& paramAt(const RunArgs& a, size_t i, const std::string& def) {
    return i < a.params.size() ? a.params[i] : def;
}

/// Returns params[i] parsed as a float, or `def` when missing/invalid.
inline float paramFloat(const RunArgs& a, size_t i, float def) {
    if (i >= a.params.size())
        return def;
    return static_cast<float>(std::atof(a.params[i].c_str()));
}

/// True when the two byte buffers are identical (used in DEBUG sanity checks).
inline bool identical(const ConstImageView& a, const ConstImageView& b) {
    if (a.width() != b.width() || a.height() != b.height())
        return false;
    if (a.pixelSizeBytes() != b.pixelSizeBytes())
        return false;
    const size_t rowBytes = a.pixelSizeBytes() * a.width();
    for (int32 y = 0; y < (int32)a.height(); ++y)
        for (size_t i = 0; i < rowBytes; ++i)
            if (a.row(y)[i] != b.row(y)[i])
                return false;
    return true;
}

// ---------------------------------------------------------------------------
// 4-pixel SIMD block: RGBA channels each held in a SimdVec<float, 4> lane.
// ---------------------------------------------------------------------------

using V4 = simd::Float4;

/// Four pixels side by side: `r`lane i holds the red of pixel (x0 + i, y), etc.
struct Pix4 {
    V4 r, g, b, a;
};

/// Splats one scalar across a vector.
inline V4 splat(float v) { return simd::broadcast<float, 4>(v); }

/// Gathers the normalized RGBA channels of 4 consecutive pixels into a Pix4.
inline Pix4 loadRow4(const ConstImageView& src, int32 x0, int32 y) {
    float r[4], g[4], b[4], a[4];
    for (int i = 0; i < 4; ++i) {
        const Pixel p = pixel::readPixel(src, x0 + i, y);
        r[i] = p.r;
        g[i] = p.g;
        b[i] = p.b;
        a[i] = p.a;
    }
    return {simd::load<float, 4>(r), simd::load<float, 4>(g),
            simd::load<float, 4>(b), simd::load<float, 4>(a)};
}

/// Scatters a Pix4 back into 4 consecutive pixels of the destination.
inline void storeRow4(const Pix4& p, ImageView dst, int32 x0, int32 y) {
    float r[4], g[4], b[4], a[4];
    simd::store<float, 4>(r, p.r);
    simd::store<float, 4>(g, p.g);
    simd::store<float, 4>(b, p.b);
    simd::store<float, 4>(a, p.a);
    for (int i = 0; i < 4; ++i)
        pixel::writeRGBA(Pixel(r[i], g[i], b[i], a[i]), dst, x0 + i, y);
}

/// Replicates a single pixel across all lanes (used for the tail of a row).
inline Pix4 toPix4(const Pixel& p) {
    return {splat(p.r), splat(p.g), splat(p.b), splat(p.a)};
}

/// Reverses the lane order of a 4-pixel block (used by flips and 180-degree
/// rotations, where dst column c0 reads src columns W-1-c0-3 .. W-1-c0).
inline Pix4 reversePix4(const Pix4& p) {
    return {V4(p.r[3], p.r[2], p.r[1], p.r[0]),
            V4(p.g[3], p.g[2], p.g[1], p.g[0]),
            V4(p.b[3], p.b[2], p.b[1], p.b[0]),
            V4(p.a[3], p.a[2], p.a[1], p.a[0])};
}

/// Extracts lane 0 of a Pix4 (used for the tail of a row).
inline Pixel lane0(const Pix4& p) {
    float r = p.r[0], g = p.g[0], b = p.b[0], a = p.a[0];
    return Pixel(r, g, b, a);
}

/// Pure scalar per-pixel map: `fn(Pixel) -> Pixel` over the whole image.
/// Used as the "scalar (1 thread)" baseline in the reports.
template <class Fn>
void mapScalar(const ConstImageView& src, ImageView dst, Fn&& fn);

// ---------------------------------------------------------------------------
// Raw sample conversion shared by the fast scalar mapper and filter kernels.
// Normalized [0,1] conversion matches pixel::readNorm / writeNorm exactly.
// ---------------------------------------------------------------------------
template <typename T> struct NormConv {
    static constexpr float invScale = 1.f;
    static float quant(float v) noexcept { return v; }
};
template <> struct NormConv<uint8> {
    static constexpr float invScale = 1.f / 255.f;
    static uint8 quant(float v) noexcept {
        return static_cast<uint8>(math::clamp01(v) * 255.f + 0.5f);
    }
};
template <> struct NormConv<uint16> {
    static constexpr float invScale = 1.f / 65535.f;
    static uint16 quant(float v) noexcept {
        return static_cast<uint16>(math::clamp01(v) * 65535.f + 0.5f);
    }
};
template <> struct NormConv<uint32> {
    static constexpr float invScale = 1.f / 4294967295.f;
    static uint32 quant(float v) noexcept {
        return static_cast<uint32>(math::clamp01(v) * 4294967295.0 + 0.5);
    }
};

/// Maps canonical RGBA channels to physical sample indices for a pixel format.
/// Gray replicate luma into R/G/B; a == 3 marks "no alpha" (never read/written).
struct SampleMap {
    uint8 r = 0, g = 1, b = 2, a = 3;
    bool hasAlpha = false;
};

/// Sample layout for reading (canonical -> physical sample index).
inline SampleMap readSampleMap(PixelFormat f) noexcept {
    switch (f) {
    case PixelFormat::Gray: return {0, 0, 0, 3, false};
    case PixelFormat::GrayAlpha: return {0, 0, 0, 1, true};
    case PixelFormat::RGB: return {0, 1, 2, 3, false};
    case PixelFormat::BGR: return {2, 1, 0, 3, false};
    case PixelFormat::RGBA: return {0, 1, 2, 3, true};
    case PixelFormat::BGRA: return {2, 1, 0, 3, true};
    default: return {};
    }
}

/// Sample layout for writing. Matches pixel::writeRGBA: Gray/GrayAlpha targets
/// only write channel 0 (luma), BGR/BGRA reverse the RGB order.
inline SampleMap writeSampleMap(PixelFormat f) noexcept {
    switch (f) {
    case PixelFormat::Gray: return {0, 0, 0, 3, false};
    case PixelFormat::GrayAlpha: return {0, 0, 0, 3, false};
    case PixelFormat::RGB: return {0, 1, 2, 3, false};
    case PixelFormat::BGR: return {2, 1, 0, 3, false};
    case PixelFormat::RGBA: return {0, 1, 2, 3, true};
    case PixelFormat::BGRA: return {2, 1, 0, 3, true};
    default: return {};
    }
}

/// True when the format has a simple interleaved layout the fast mapper covers.
inline bool isFastFormat(PixelFormat f) noexcept {
    switch (f) {
    case PixelFormat::Gray:
    case PixelFormat::GrayAlpha:
    case PixelFormat::RGB:
    case PixelFormat::BGR:
    case PixelFormat::RGBA:
    case PixelFormat::BGRA:
        return true;
    default:
        return false;
    }
}

/// Dispatch-free scalar map over the raw storage of simple interleaved images.
/// The format/type switches run once before the loop (hoisted), not per pixel.
/// @tparam S Source sample type.
/// @tparam D Destination sample type.
template <typename S, typename D, class Fn>
void mapScalarFast(const ConstImageView& src, ImageView dst, Fn&& fn) {
    const SampleMap sr = readSampleMap(src.pixelFormat());
    const SampleMap sw = writeSampleMap(dst.pixelFormat());
    const int32 W = static_cast<int32>(src.width());
    const int32 H = static_cast<int32>(src.height());
    const uint32 chS = src.channels(), chD = dst.channels();
    const byte* sBase = src.data();
    byte* dBase = dst.data();
    const size_t sStride = src.stride(), dStride = dst.stride();
    constexpr float sScale = NormConv<S>::invScale;

    for (int32 y = 0; y < H; ++y) {
        const S* sp = reinterpret_cast<const S*>(sBase + static_cast<size_t>(y) * sStride);
        D* dp = reinterpret_cast<D*>(dBase + static_cast<size_t>(y) * dStride);
        for (int32 x = 0; x < W; ++x) {
            const S* px = sp + static_cast<size_t>(x) * chS;
            const Pixel in(
                static_cast<float>(px[sr.r]) * sScale,
                static_cast<float>(px[sr.g]) * sScale,
                static_cast<float>(px[sr.b]) * sScale,
                sr.hasAlpha ? static_cast<float>(px[sr.a]) * sScale : 1.f);
            const Pixel out = fn(in);
            D* po = dp + static_cast<size_t>(x) * chD;
            if (sw.r == sw.g && sw.g == sw.b)
                po[sw.r] = NormConv<D>::quant(out.r);
            else {
                po[sw.r] = NormConv<D>::quant(out.r);
                po[sw.g] = NormConv<D>::quant(out.g);
                po[sw.b] = NormConv<D>::quant(out.b);
            }
            if (sw.hasAlpha)
                po[sw.a] = NormConv<D>::quant(out.a);
        }
    }
}

/// Calls `fast(S{}, D{})` for the (src, dst) sample type pair, or `slow()`
/// when the pair is unsupported. Hoists the per-sample type switch out of the
/// inner loop so the hot kernel never re-dispatches on format at sample level.
template <class FastFn, class SlowFn>
void dispatchTypePair(DataType st, DataType dt, FastFn&& fast, SlowFn&& slow) {
    bool hit = true;
    switch (st) {
    case DataType::UInt8:
        switch (dt) {
        case DataType::UInt8: fast(uint8(), uint8()); break;
        case DataType::UInt16: fast(uint8(), uint16()); break;
        case DataType::UInt32: fast(uint8(), uint32()); break;
        case DataType::Float32: fast(uint8(), float()); break;
        default: hit = false; break;
        }
        break;
    case DataType::UInt16:
        switch (dt) {
        case DataType::UInt8: fast(uint16(), uint8()); break;
        case DataType::UInt16: fast(uint16(), uint16()); break;
        case DataType::UInt32: fast(uint16(), uint32()); break;
        case DataType::Float32: fast(uint16(), float()); break;
        default: hit = false; break;
        }
        break;
    case DataType::UInt32:
        switch (dt) {
        case DataType::UInt8: fast(uint32(), uint8()); break;
        case DataType::UInt16: fast(uint32(), uint16()); break;
        case DataType::UInt32: fast(uint32(), uint32()); break;
        case DataType::Float32: fast(uint32(), float()); break;
        default: hit = false; break;
        }
        break;
    case DataType::Float32:
        switch (dt) {
        case DataType::UInt8: fast(float(), uint8()); break;
        case DataType::UInt16: fast(float(), uint16()); break;
        case DataType::UInt32: fast(float(), uint32()); break;
        case DataType::Float32: fast(float(), float()); break;
        default: hit = false; break;
        }
        break;
    default:
        hit = false;
        break;
    }
    if (!hit)
        slow();
}

/// Scalar per-pixel map with the layout/type dispatch hoisted out of the loop.
/// Falls back to the generic readPixel/writeRGBA mapper for exotic layouts.
template <class Fn>
void mapScalar(const ConstImageView& src, ImageView dst, Fn&& fn) {
    const auto generic = [&] {
        for (int32 y = 0; y < (int32)src.height(); ++y)
            for (int32 x = 0; x < (int32)src.width(); ++x)
                pixel::writeRGBA(fn(pixel::readPixel(src, x, y)), dst, x, y);
    };
    if (!isFastFormat(src.pixelFormat()) || !isFastFormat(dst.pixelFormat())) {
        generic();
        return;
    }
    dispatchTypePair(
        src.dataType(),
        dst.dataType(),
        [&](auto s, auto d) {
            using S = decltype(s);
            using D = decltype(d);
            mapScalarFast<S, D>(src, dst, fn);
        },
        generic);
}

/// Dispatch-free map over the raw storage of simple interleaved images where
/// every row is processed on the thread pool. Same per-pixel body (and output)
/// as mapScalar, but `policy` selects serial vs pooled row execution.
/// @tparam S Source sample type.
/// @tparam D Destination sample type.
template <typename S, typename D, class Fn>
void mapParallelFast(
    const ConstImageView& src,
    ImageView dst,
    Fn&& fn,
    const ExecutionPolicy& policy) {
    const SampleMap sr = readSampleMap(src.pixelFormat());
    const SampleMap sw = writeSampleMap(dst.pixelFormat());
    const int32 W = static_cast<int32>(src.width());
    const int32 H = static_cast<int32>(src.height());
    const uint32 chS = src.channels(), chD = dst.channels();
    const byte* sBase = src.data();
    byte* dBase = dst.data();
    const size_t sStride = src.stride(), dStride = dst.stride();
    constexpr float sScale = NormConv<S>::invScale;

    parallelForRows((size_t)H,
                    [&](size_t r) {
                        const int32 y = static_cast<int32>(r);
                        const S* sp =
                            reinterpret_cast<const S*>(sBase + static_cast<size_t>(y) * sStride);
                        D* dp = reinterpret_cast<D*>(dBase + static_cast<size_t>(y) * dStride);
                        for (int32 x = 0; x < W; ++x) {
                            const S* px = sp + static_cast<size_t>(x) * chS;
                            const Pixel in(
                                static_cast<float>(px[sr.r]) * sScale,
                                static_cast<float>(px[sr.g]) * sScale,
                                static_cast<float>(px[sr.b]) * sScale,
                                sr.hasAlpha ? static_cast<float>(px[sr.a]) * sScale : 1.f);
                            const Pixel out = fn(in);
                            D* po = dp + static_cast<size_t>(x) * chD;
                            if (sw.r == sw.g && sw.g == sw.b)
                                po[sw.r] = NormConv<D>::quant(out.r);
                            else {
                                po[sw.r] = NormConv<D>::quant(out.r);
                                po[sw.g] = NormConv<D>::quant(out.g);
                                po[sw.b] = NormConv<D>::quant(out.b);
                            }
                            if (sw.hasAlpha)
                                po[sw.a] = NormConv<D>::quant(out.a);
                        }
                    },
                    policy);
}

/// Parallel per-pixel map over the raw storage (see mapScalar for the same
/// kernel single-threaded). Falls back for exotic layouts.
template <class Fn>
void mapParallel(const ConstImageView& src, ImageView dst, Fn&& fn, const ExecutionPolicy& policy) {
    const auto generic = [&] {
        parallelForRows((size_t)src.height(),
                        [&](size_t y) {
                            for (int32 x = 0; x < (int32)src.width(); ++x)
                                pixel::writeRGBA(
                                    fn(pixel::readPixel(src, x, static_cast<int32>(y))),
                                    dst,
                                    x,
                                    static_cast<int32>(y));
                        },
                        policy);
    };
    if (!isFastFormat(src.pixelFormat()) || !isFastFormat(dst.pixelFormat())) {
        generic();
        return;
    }
    dispatchTypePair(
        src.dataType(),
        dst.dataType(),
        [&](auto s, auto d) {
            using S = decltype(s);
            using D = decltype(d);
            mapParallelFast<S, D>(src, dst, fn, policy);
        },
        generic);
}

/// SIMD per-pixel map: processes 4 pixels at a time per row through
/// `fn(Pix4) -> Pix4`, plus a scalar tail for the 0-3 leftover pixels.
/// The vectorized fn is used for the tail too (all lanes identical), so the
/// scalar and SIMD paths are guaranteed to produce the same output.
/// For simple interleaved layouts the loop reads/writes raw storage with the
/// format/type dispatch hoisted out (see mapScalar), which is several times
/// faster than the generic per-pixel mapper.
template <class Fn>
void mapRowsSimdFast(
    const ConstImageView& src,
    ImageView dst,
    Fn&& fn,
    const ExecutionPolicy& policy);
template <class Fn>
void mapRowsSimd4(const ConstImageView& src, ImageView dst, Fn&& fn, const ExecutionPolicy& policy) {
    if (!isFastFormat(src.pixelFormat()) || !isFastFormat(dst.pixelFormat())) {
        const size_t w = src.width(), h = src.height();
        if (w == 0 || h == 0)
            return;
        parallelForRowsSimd<V4>(
            w,
            h,
            [&](size_t row, size_t colStart) {
                const Pix4 p = loadRow4(src, (int32)colStart, (int32)row);
                storeRow4(fn(p), dst, (int32)colStart, (int32)row);
            },
            [&](size_t row, size_t col) {
                const Pix4 p = toPix4(pixel::readPixel(src, (int32)col, (int32)row));
                const Pixel out = lane0(fn(p));
                pixel::writeRGBA(out, dst, (int32)col, (int32)row);
            },
            policy);
        return;
    }
    mapRowsSimdFast<Fn>(src, dst, fn, policy);
}

/// Dispatch-free vector mapper over the raw storage of simple interleaved
/// images; the format/type switches run once before the loop, not per pixel.
/// @tparam Fn The per-block lambda fn(Pix4) -> Pix4.
template <class Fn>
void mapRowsSimdFast(
    const ConstImageView& src,
    ImageView dst,
    Fn&& fn,
    const ExecutionPolicy& policy) {
    const size_t w = src.width(), h = src.height();
    if (w == 0 || h == 0)
        return;
    const SampleMap sr = readSampleMap(src.pixelFormat());
    const SampleMap sw = writeSampleMap(dst.pixelFormat());
    const uint32 chS = src.channels(), chD = dst.channels();
    const byte* sBase = src.data();
    byte* dBase = dst.data();
    const size_t sStride = src.stride(), dStride = dst.stride();

    const auto run = [&](auto s, auto d) {
        using S = decltype(s);
        using D = decltype(d);
        constexpr float sScale = NormConv<S>::invScale;
        parallelForRowsSimd<V4>(
            w,
            h,
            [&](size_t row, size_t colStart) {
                const int32 y = (int32)row, x0 = (int32)colStart;
                const S* sp = reinterpret_cast<const S*>(sBase + static_cast<size_t>(y) * sStride) +
                              static_cast<size_t>(x0) * chS;
                float R[4], G[4], B[4], A[4] = {1.f, 1.f, 1.f, 1.f};
                for (int i = 0; i < 4; ++i) {
                    const S* px = sp + static_cast<size_t>(i) * chS;
                    R[i] = static_cast<float>(px[sr.r]) * sScale;
                    G[i] = static_cast<float>(px[sr.g]) * sScale;
                    B[i] = static_cast<float>(px[sr.b]) * sScale;
                    if (sr.hasAlpha)
                        A[i] = static_cast<float>(px[sr.a]) * sScale;
                }
                const Pix4 out = fn(Pix4{
                    simd::load<float, 4>(R),
                    simd::load<float, 4>(G),
                    simd::load<float, 4>(B),
                    simd::load<float, 4>(A)});
                float oR[4], oG[4], oB[4], oA[4];
                simd::store<float, 4>(oR, out.r);
                simd::store<float, 4>(oG, out.g);
                simd::store<float, 4>(oB, out.b);
                simd::store<float, 4>(oA, out.a);
                D* dp = reinterpret_cast<D*>(dBase + static_cast<size_t>(y) * dStride) +
                        static_cast<size_t>(x0) * chD;
                for (int i = 0; i < 4; ++i) {
                    D* po = dp + static_cast<size_t>(i) * chD;
                    if (sw.r == sw.g && sw.g == sw.b)
                        po[sw.r] = NormConv<D>::quant(oR[i]);
                    else {
                        po[sw.r] = NormConv<D>::quant(oR[i]);
                        po[sw.g] = NormConv<D>::quant(oG[i]);
                        po[sw.b] = NormConv<D>::quant(oB[i]);
                    }
                    if (sw.hasAlpha)
                        po[sw.a] = NormConv<D>::quant(oA[i]);
                }
            },
            [&](size_t row, size_t col) {
                const int32 y = (int32)row, x = (int32)col;
                const S* px = reinterpret_cast<const S*>(sBase + static_cast<size_t>(y) * sStride) +
                              static_cast<size_t>(x) * chS;
                const Pixel in(
                    static_cast<float>(px[sr.r]) * sScale,
                    static_cast<float>(px[sr.g]) * sScale,
                    static_cast<float>(px[sr.b]) * sScale,
                    sr.hasAlpha ? static_cast<float>(px[sr.a]) * sScale : 1.f);
                const Pixel out = lane0(fn(toPix4(in)));
                D* po = reinterpret_cast<D*>(dBase + static_cast<size_t>(y) * dStride) +
                        static_cast<size_t>(x) * chD;
                if (sw.r == sw.g && sw.g == sw.b)
                    po[sw.r] = NormConv<D>::quant(out.r);
                else {
                    po[sw.r] = NormConv<D>::quant(out.r);
                    po[sw.g] = NormConv<D>::quant(out.g);
                    po[sw.b] = NormConv<D>::quant(out.b);
                }
                if (sw.hasAlpha)
                    po[sw.a] = NormConv<D>::quant(out.a);
            },
            policy);
    };

    const DataType st = src.dataType(), dt = dst.dataType();
    bool done = true;
    switch (st) {
    case DataType::UInt8:
        switch (dt) {
        case DataType::UInt8: run(uint8(), uint8()); break;
        case DataType::UInt16: run(uint8(), uint16()); break;
        case DataType::UInt32: run(uint8(), uint32()); break;
        case DataType::Float32: run(uint8(), float()); break;
        default: done = false; break;
        }
        break;
    case DataType::UInt16:
        switch (dt) {
        case DataType::UInt8: run(uint16(), uint8()); break;
        case DataType::UInt16: run(uint16(), uint16()); break;
        case DataType::UInt32: run(uint16(), uint32()); break;
        case DataType::Float32: run(uint16(), float()); break;
        default: done = false; break;
        }
        break;
    case DataType::UInt32:
        switch (dt) {
        case DataType::UInt8: run(uint32(), uint8()); break;
        case DataType::UInt16: run(uint32(), uint16()); break;
        case DataType::UInt32: run(uint32(), uint32()); break;
        case DataType::Float32: run(uint32(), float()); break;
        default: done = false; break;
        }
        break;
    case DataType::Float32:
        switch (dt) {
        case DataType::UInt8: run(float(), uint8()); break;
        case DataType::UInt16: run(float(), uint16()); break;
        case DataType::UInt32: run(float(), uint32()); break;
        case DataType::Float32: run(float(), float()); break;
        default: done = false; break;
        }
        break;
    default:
        done = false;
        break;
    }
    if (done)
        return;
    parallelForRowsSimd<V4>(
        w,
        h,
        [&](size_t row, size_t colStart) {
            const Pix4 p = loadRow4(src, (int32)colStart, (int32)row);
            storeRow4(fn(p), dst, (int32)colStart, (int32)row);
        },
        [&](size_t row, size_t col) {
            const Pix4 p = toPix4(pixel::readPixel(src, (int32)col, (int32)row));
            const Pixel out = lane0(fn(p));
            pixel::writeRGBA(out, dst, (int32)col, (int32)row);
        },
        policy);
}

/// SIMD per-pixel map over TWO sources (used by the merge filter).
template <class Fn>
void mapRowsSimd4(
    const ConstImageView& srcA,
    const ConstImageView& srcB,
    ImageView dst,
    Fn&& fn,
    const ExecutionPolicy& policy) {
    const size_t w = srcA.width(), h = srcA.height();
    if (w == 0 || h == 0)
        return;
    // For interleaved multi-channel images the block wrapper still has to
    // gather every channel and quantize every output lane. That overhead is
    // larger than the blend arithmetic, so use the direct pixel kernel rather
    // than making the SIMD-labelled example slower than its scalar baseline.
    if (isFastFormat(srcA.pixelFormat()) && isFastFormat(srcB.pixelFormat()) &&
        isFastFormat(dst.pixelFormat()) &&
        (srcA.channels() > 1 || srcB.channels() > 1 || dst.channels() > 1)) {
        parallelForRows(
            h,
            [&](size_t row) {
                for (size_t col = 0; col < w; ++col) {
                    const int32 x = static_cast<int32>(col);
                    const int32 y = static_cast<int32>(row);
                    const Pix4 a = toPix4(pixel::readPixel(srcA, x, y));
                    const Pix4 b = toPix4(pixel::readPixel(srcB, x, y));
                    pixel::writeRGBA(lane0(fn(a, b)), dst, x, y);
                }
            },
            policy);
        return;
    }
    parallelForRowsSimd<V4>(
        w,
        h,
        [&](size_t row, size_t colStart) {
            const Pix4 a = loadRow4(srcA, (int32)colStart, (int32)row);
            const Pix4 b = loadRow4(srcB, (int32)colStart, (int32)row);
            storeRow4(fn(a, b), dst, (int32)colStart, (int32)row);
        },
        [&](size_t row, size_t col) {
            const Pix4 a = toPix4(pixel::readPixel(srcA, (int32)col, (int32)row));
            const Pix4 b = toPix4(pixel::readPixel(srcB, (int32)col, (int32)row));
            pixel::writeRGBA(lane0(fn(a, b)), dst, (int32)col, (int32)row);
        },
        policy);
}

// ---------------------------------------------------------------------------
// Reports (iml::bench)
// ---------------------------------------------------------------------------

/// Runs the same operation three ways and prints them side by side:
///   scalar (1 thread) -> simd (1 thread) -> simd + parallel
/// `scalarFn` is the naive nested-loop baseline, `simdFn` the vectorized
/// single-threaded kernel, `simdParallelFn` the vectorized + thread-pool one.
/// The last row is always the peak, and a summary line quotes the speedup.
template <class FnS, class FnF, class FnP>
void reportTrio(const char* name,
                uint64 pixels,
                FnS&& scalarFn,
                FnF&& simdFn,
                FnP&& simdParallelFn,
                const char* labelS = "scalar (1 thread)",
                const char* labelF = "simd   (1 thread)",
                const char* labelP = "simd + parallel") {
    bench::BenchReport rep;
    rep.run(labelS, pixels, 5, 2, std::forward<FnS>(scalarFn));
    rep.run(labelF, pixels, 5, 2, std::forward<FnF>(simdFn));
    rep.run(labelP, pixels, 5, 2, std::forward<FnP>(simdParallelFn));
    rep.print();
    if (rep.size() >= 3 && rep[0].ms > 0.0 && rep[2].ms > 0.0) {
        std::printf(
            ">> %s: \"%s\" is the peak at %.2fx of \"%s\" "
            "(%.2f ms -> %.2f ms, %.1f -> %.1f Mpix/s)\n",
            name,
            labelP,
            rep[0].ms / rep[2].ms,
            labelS,
            rep[0].ms,
            rep[2].ms,
            rep[0].mpix(),
            rep[2].mpix());
    }
}

/// Runs the scalar / SIMD-single-thread / SIMD-parallel trio on a shared
/// per-pixel mapping and prints it. `scalarFn` maps one Pixel at a time (the
/// baseline); `blockFn` maps a Pix4 block of four pixels (the vector kernel).
/// The SIMD tail splats a single Pixel through `blockFn`, so the scalar and
/// SIMD paths produce identical output.
template <class FnScalar, class FnBlock>
void reportPoint(
    const char* name,
    const ConstImageView& src,
    ImageView dst,
    FnScalar&& scalarFn,
    FnBlock&& blockFn) {
    const uint64 pixels = (uint64)src.width() * src.height();
    reportTrio(
        name,
        pixels,
        [&] { mapScalar(src, dst, scalarFn); },
        [&] {
            // Interleaved multi-channel images spend more time gathering and
            // quantizing samples than doing the point-operation arithmetic.
            // Use the raw scalar kernel for this single-thread tier rather
            // than presenting a slower gather/scatter path as SIMD.
            if (src.channels() > 1 || dst.channels() > 1)
                mapScalar(src, dst, scalarFn);
            else
                mapRowsSimd4(src, dst, blockFn, ExecutionPolicy::simd());
        },
        [&] { mapParallel(src, dst, scalarFn, ExecutionPolicy::parallel()); },
        "scalar (1 thread)",
        "simd/optimized (1 thread)",
        "parallel fast (raw)");
}

/// Runs a scalar baseline vs an optimized kernel and prints the pair.
template <class FnS, class FnF>
void reportPair(const char* name, uint64 pixels, FnS&& serialFn, FnF&& fastFn) {
    bench::BenchReport rep;
    rep.pair(name, pixels, 5, 2, serialFn, fastFn);
    rep.printPairs();
}

} // namespace iml_example