// examples/processing_demo.cpp
//
// Demonstrates the processing + procedural pipeline: loads an image, applies
// a Gaussian blur (larger image = visually obvious), runs a Sobel edge
// filter, generates a procedural Perlin-noise texture and a heightmap, and
// saves everything side by side as PNGs.
//
// Build:
//   mingw32-make -C build
// Run:
//   bin/processing_demo.exe assets/building.jpg

#include <iostream>
#include <string>

#include "imagelib/imagelib.h"

using namespace iml;

namespace {

void saveAsGray(const iml::Image& gray, const std::string& label) {
    gray.save(label);
    std::cout << "Saved " << label << "\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: processing_demo <input-image>\n";
        std::cout << "  Writes blur.png, sobel.png, noise.png, heightmap.png next to the\n";
        std::cout << "  working directory.\n";
        return 1;
    }

    iml::Image src(argv[1]);
    const uint32 w = src.width(), h = src.height();
    std::cout << "Input: " << w << "x" << h << "\n";

    // --- Grayscale working image -------------------------------------------
    iml::Image gray(iml::fmt::gray32f, w, h, iml::ColorSpace::Gray);
    iml::proc::color::toGray(src.view(), gray.view());

    // --- Gaussian blur (float preserve, then save via 8-bit) ----------------
    iml::Image blurred(iml::fmt::gray32f, w, h, iml::ColorSpace::Gray);
    iml::proc::conv::gaussianBlur(gray.view(), blurred.view(), 2.0f);
    iml::Image blur8(iml::fmt::gray8, w, h, iml::ColorSpace::Gray);
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x) {
            float v = blurred.get<float>(x, y, 0);
            v = iml::math::saturate(v) * 255.f;
            blur8.set<uint8_t>(x, y, 0, static_cast<uint8_t>(v + 0.5f));
        }
    saveAsGray(blur8, "blur.png");

    // --- Sobel magnitude -----------------------------------------------------
    iml::Image gx(iml::fmt::gray32f, w, h, iml::ColorSpace::Gray);
    iml::Image gy(iml::fmt::gray32f, w, h, iml::ColorSpace::Gray);
    iml::proc::conv::convolve(gray.view(), gx.view(), iml::proc::conv::sobelXKernel());
    iml::proc::conv::convolve(gray.view(), gy.view(), iml::proc::conv::sobelYKernel());
    iml::Image edge(iml::fmt::gray8, w, h, iml::ColorSpace::Gray);
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x) {
            float g = std::sqrt(std::fma(gx.get<float>(x, y, 0),
                                        gx.get<float>(x, y, 0),
                                        gy.get<float>(x, y, 0) * gy.get<float>(x, y, 0)));
            edge.set<uint8_t>(x, y, 0, static_cast<uint8_t>(std::min(g, 1.f) * 255.f));
        }
    saveAsGray(edge, "sobel.png");

    // --- Procedural Perlin noise texture ------------------------------------
    iml::Image noise(iml::fmt::rgb8, 256, 256, iml::ColorSpace::SRGB);
    iml::procgen::fillNoise(noise.view(), iml::procgen::NoiseType::Perlin, 2024, 4.f);
    noise.save("noise.png");
    std::cout << "Saved noise.png (256x256 Perlin)\n";

    // --- Heightmap as 8-bit + normal map ------------------------------------
    iml::Image hm(iml::fmt::gray8, 256, 256, iml::ColorSpace::Gray);
    iml::procgen::fillNoise(hm.view(), iml::procgen::NoiseType::Fbm, 7, 3.f);
    saveAsGray(hm, "heightmap.png");

    iml::Image hmf(iml::fmt::gray32f, 256, 256, iml::ColorSpace::Gray);
    for (uint32 y = 0; y < 256; ++y)
        for (uint32 x = 0; x < 256; ++x)
            hmf.set<float>(x, y, 0, hm.get<uint8_t>(x, y, 0) / 255.f);
    iml::Image nm(iml::fmt::rgb32f, 256, 256, iml::ColorSpace::SRGB);
    iml::procgen::heightmapNormal(hmf.view(), nm.view(), 2.f);
    iml::Image nm8(iml::fmt::rgb8, 256, 256, iml::ColorSpace::SRGB);
    for (uint32 y = 0; y < 256; ++y)
        for (uint32 x = 0; x < 256; ++x)
            for (uint32 c = 0; c < 3; ++c)
                nm8.set<uint8_t>(x, y, c, static_cast<uint8_t>(iml::math::saturate(nm.get<float>(x, y, c)) * 255.f));
    nm8.save("normalmap.png");
    std::cout << "Saved normalmap.png\n";

    std::cout << "Done.\n";
    return 0;
}