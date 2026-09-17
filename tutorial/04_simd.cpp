// ImageLib Tutorial 04 - SIMD without the pain
// ---------------------------------------------
// "SIMD" = one CPU instruction working on several numbers at once. ImageLib
// does this for you automatically on the fast paths (ExecutionPolicy::simd*),
// and exposes two high-level APIs if you want to help it:
//   * iml::simd::SimdVec / load / store  - tiny portable vector type
//   * parallelForSimd / parallelForRowsSimd - SIMD loops with thread pooling
//
// Build + run (from the repository root):
//   g++ -std=c++17 -O2 -Iinclude -Isrc tutorial/04_simd.cpp -Lbin \
//       -limagelib -lstb_image -o bin/tutorial_04_simd.exe
//   ./bin/tutorial_04_simd.exe [output_folder]
//
// Tip: rebuild the library with -DIML_ENABLE_NATIVE=ON to unlock AVX2/AVX-512.

#include "imagelib/imagelib.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace iml;   // now "simd" means iml::simd

namespace {

std::string outPath(const std::string& folder, const std::string& file) {
    return folder.empty() ? file : folder + "/" + file;
}

void printIsaInfo() {
    std::cout << "--- what this CPU can do ---\n";
    std::cout << "SIMD available : " << (simd::simdAvailable() ? "yes" : "no") << '\n';
    std::cout << "active ISA     : " << simd::activeIsaName() << '\n';
    std::cout << "vector width   : " << simd::vectorBytes() << " bytes per operation\n";
    std::cout << "float lanes    : " << simd::simdWidth<float>() << " floats at once\n";

    const simd::SimdFeatures assured = simd::assuredFeatures();
    std::cout << "assured        :"
              << " SSE2=" << (assured.has(simd::Sse2) ? "on" : "off")
              << " AVX2=" << (assured.has(simd::Avx2) ? "on" : "off")
              << " AVX512=" << (assured.has(simd::Avx512F) ? "on" : "off")
              << " NEON=" << (assured.has(simd::Neon) ? "on" : "off") << '\n';
    std::cout << '\n';
}

void demoSimdVec() {
    std::cout << "--- iml::simd::Vec: process 4 floats like one value ---\n";
    simd::Float4 a(1.0f, 0.5f, 0.0f, -0.25f);      // four lanes
    simd::Float4 b(0.25f, 0.25f, 0.25f, 0.25f);

    simd::Float4 v = a * 2.0f + b;                  // arithmetic on all lanes
    v = simd::clamp01(v);                           // keep lanes in [0,1]

    float data[4];
    simd::store(data, v);                           // store lanes to memory
    std::cout << "a*2+b clamped = " << data[0] << ", " << data[1] << ", " << data[2]
              << ", " << data[3] << '\n';
    std::cout << "horizontal sum = " << simd::sum(v) << '\n';
    std::cout << "dot with itself = " << simd::dot(v, v) << '\n';

    // load() reads 4 consecutive floats from a plain array
    const float src[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    simd::Float4 loaded = simd::load<float, 4>(src);
    std::cout << "loaded[2] = " << loaded.z() << "   (lanes named x y z w)\n";
    std::cout << '\n';
}

void demoSaturatingOps() {
    std::cout << "--- ready-made byte helpers (choose the fastest ISA for you) ---\n";
    std::vector<uint8> a(64), b(64), out(64);
    for (size_t i = 0; i < a.size(); ++i) {
        a[i] = (uint8)(i * 3);
        b[i] = (uint8)(250 - i * 2);
    }
    simd::averageU8(out.data(), a.data(), b.data(), a.size());      // (a+b+1)/2
    std::cout << "average of first bytes: " << (int)out[1] << " (expect "
              << (int)((a[1] + b[1] + 1) / 2) << ")\n";
    simd::saturatingAddU8(out.data(), a.data(), b.data(), a.size()); // a+b, capped at 255
    std::cout << "saturating add last   : " << (int)out[63] << " (expect 255, it caps)\n";
    std::cout << '\n';
}

void demoScalarFallback() {
    std::cout << "--- iml::simd::split: align start + vectors + tail ---\n";
    const simd::SimdSplit s = simd::split<float>(21, nullptr);
    std::cout << "21 floats with no base address -> "
              << s.vectorElems / 4 << " full vectors + " << s.tail << " tail elements\n";
    std::cout << '\n';
}

// Converts a procedurally colored gradient to grayscale: three contiguous
// float planes (red / green / blue, built once) are reduced to a luma plane
// with the Rec.709 math running on 4 values per vector block, with a scalar
// fallback for leftover pixels. This IS the grayscale filter from Tutorial 1 —
// the same math, only moved onto SIMD registers.
Image demoGraySimd() {
    const uint32 w = 256, h = 256;
    const size_t n = (size_t)w * h;
    std::vector<float> red(n), green(n), blue(n), luma(n);
    for (uint32 y = 0; y < h; ++y) {
        for (uint32 x = 0; x < w; ++x) {
            const float u = (float)x / (float)(w - 1);
            const float v = (float)y / (float)(h - 1);
            const size_t i = (size_t)y * w + x;
            red[i] = u;                                    // horizontal gradient
            green[i] = v;                                  // vertical gradient
            blue[i] = 0.5f * u + 0.5f * v;                 // blend of both
        }
    }

    parallelForRowsSimd<simd::Float4>(
        w,
        h,
        // Vector path: called once per 4-wide block of a row.
        [&](size_t row, size_t colStart) {
            const size_t off = row * w + colStart;
            const simd::Float4 r = simd::load<float, 4>(&red[off]);
            const simd::Float4 g = simd::load<float, 4>(&green[off]);
            const simd::Float4 b = simd::load<float, 4>(&blue[off]);
            simd::store<float, 4>(&luma[off],
                                  simd::clamp01(r * 0.2126f + g * 0.7152f + b * 0.0722f));
        },
        // Scalar path: called for the 0-3 leftover pixels at the row end.
        [&](size_t row, size_t col) {
            const size_t off = row * w + col;
            luma[off] = iml::math::clamp01(red[off] * 0.2126f + green[off] * 0.7152f +
                                           blue[off] * 0.0722f);
        },
        ExecutionPolicy::simdParallel());

    Image out(fmt::gray8, w, h, ColorSpace::Gray);
    ImageView dv = out.view();
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            pixel::writeNorm(dv, (int32)x, (int32)y, 0, luma[(size_t)y * w + x]);
    return out;
}

} // namespace

int main(int argc, char** argv) {
    const std::string outDir = argc > 1 ? argv[1] : "";

    printIsaInfo();
    demoSimdVec();
    demoSaturatingOps();
    demoScalarFallback();

    try {
        const Image img = demoGraySimd();
        img.save(outPath(outDir, "04_simd.png"));
        std::cout << "Saved : " << outPath(outDir, "04_simd.png") << '\n';
        std::cout << "(a procedural color gradient, reduced to Rec.709 grayscale\n";
        std::cout << " through parallelForRowsSimd - the gray filter from Tutorial 1)\n";
        return 0;
    } catch (const Error& e) {
        std::cerr << "ImageLib error: " << e.what() << '\n';
        return 1;
    }
}