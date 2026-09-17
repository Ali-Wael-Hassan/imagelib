// ImageLib Tutorial 02 - Math for images
// ---------------------------------------
// The math you actually use in production image code, all from
//   iml::math : Vec2/Vec3/Vec4, scalar helpers, Rng, Mat2/Mat3
// This demo prints how the helpers behave, then uses them to *draw* pictures.
//
// Build + run (from the repository root):
//   g++ -std=c++17 -O2 -Iinclude -Isrc tutorial/02_math.cpp -Lbin \
//       -limagelib -lstb_image -o bin/tutorial_02_math.exe
//   ./bin/tutorial_02_math.exe [output_folder]

#include "imagelib/imagelib.h"

#include <cmath>
#include <iostream>
#include <string>

using namespace iml;
using namespace iml::math;

namespace {

// Blend two RGBA pixels by a factor t in [0, 1].
Pixel mix(const Pixel& a, const Pixel& b, float t) { return a * (1.f - t) + b * t; }

std::string outPath(const std::string& folder, const std::string& file) {
    return folder.empty() ? file : folder + "/" + file;
}

void printScalarHelpers() {
    std::cout << "--- scalar helpers (iml::math) ---\n";
    std::cout << "clamp(-0.2, 0, 1)   = " << clamp(-0.2f, 0.f, 1.f) << '\n';
    std::cout << "clamp(1.7, 0, 1)    = " << clamp(1.7f, 0.f, 1.f) << '\n';
    std::cout << "saturate(1.2)       = " << saturate(1.2f) << '\n';
    std::cout << "lerp(0, 1, 0.25)    = " << lerp(0.f, 1.f, 0.25f) << '\n';
    std::cout << "remap(0.5, 0,1,10,20)= " << remap(0.5f, 0.f, 1.f, 10.f, 20.f) << '\n';
    std::cout << "smoothstep(0,1,0.5) = " << smoothstep(0.f, 1.f, 0.5f) << '\n';
    std::cout << "toDegrees(pi/2)     = " << toDegrees(halfPi) << '\n';
    std::cout << "toRadians(90)       = " << toRadians(90.f) << '\n';
    std::cout << '\n';
}

void printVectors() {
    std::cout << "--- vectors (iml::math) ---\n";
    Vec2 a(3.f, 4.f);
    std::cout << "a            = (" << a.x << ", " << a.y << ")\n";
    std::cout << "a.length()   = " << a.length() << "   (expect 5)\n";
    std::cout << "a.normalized()= (" << a.normalized().x << ", " << a.normalized().y << ")\n";

    Vec2 u(1.f, 0.f), v(0.f, 1.f);
    std::cout << "dot((1,0),(0,1)) = " << dot(u, v) << "   (expect 0, they are perpendicular)\n";

    Vec3 p(1.f, 0.f, 0.f), q(0.f, 1.f, 0.f);
    Vec3 cr = cross(p, q);
    std::cout << "cross((1,0,0),(0,1,0)) = (" << cr.x << ", " << cr.y << ", " << cr.z << ")\n";

    Vec2 head(0.f, 0.f), tail(3.f, 4.f);
    std::cout << "distance(origin, (3,4)) = " << distance(head, tail) << '\n';
    std::cout << '\n';
}

void printMatrices() {
    std::cout << "--- matrices (iml::math) ---\n";
    const Mat2 rot = Mat2::rotation(piF * 0.25f);   // 45 degrees
    const Vec2 r = rot * Vec2(1.f, 0.f);
    std::cout << "rotate (1,0) by 45 deg = (" << r.x << ", " << r.y
              << ")   (y-down: clockwise on screen, (0.7071, -0.7071))\n";

    const Mat2 sc = Mat2::scale(Vec2(2.f, 3.f));
    const Vec2 s = sc * Vec2(1.f, 1.f);
    std::cout << "scale (1,1) by (2,3)   = (" << s.x << ", " << s.y << ")\n";

    const Mat3 m = Mat3::identity();
    std::cout << "identity()(0,0) = " << m(0, 0) << " (column-major indices are (col, row))\n";
    std::cout << '\n';
}

void printRandom() {
    std::cout << "--- random numbers (iml::math::Rng) ---\n";
    Rng rng(1234u);   // same seed => same sequence. Deterministic = testable.
    std::cout << "nextFloat01()  = " << rng.nextFloat01() << '\n';
    std::cout << "nextInt(1,6)   = " << rng.nextInt(1, 6) << "  (dice roll)\n";
    std::cout << "nextFloat(-1,1)= " << rng.nextFloat(-1.f, 1.f) << '\n';
    Vec2 rand = rng.nextVec2();
    std::cout << "nextVec2()     = (" << rand.x << ", " << rand.y << ")\n";
    std::cout << '\n';
}

// Draws a shaded circle on a gradient background. Every pixel value is
// computed from Vec2 distances and smoothstep: the "math" doing the drawing.
Image drawCircle() {
    const uint32 w = 512, h = 512;
    Image canvas(fmt::rgba8, w, h, ColorSpace::SRGB);
    ImageView dv = canvas.view();

    const Vec2 center(256.f, 256.f);
    const float radius = 180.f, edge = 28.f;
    const Pixel inside(1.f, 0.85f, 0.25f);    // warm yellow
    const Pixel outside(0.06f, 0.18f, 0.55f); // deep blue

    for (uint32 y = 0; y < h; ++y) {
        for (uint32 x = 0; x < w; ++x) {
            const float d = Vec2((float)x, (float)y).distance(center);
            const float t = smoothstep(radius - edge, radius + edge, d);   // 0 inside, 1 outside
            const float glow = 1.f - clamp(d / radius, 0.f, 1.f);

            Pixel p = mix(outside, inside, 1.f - t);   // blend by circle edge
            p = mix(p, Pixel(1.f, 1.f, 1.f), glow * 0.35f);  // soft highlight
            pixel::writeRGBA(p, dv, (int32)x, (int32)y);
        }
    }
    return canvas;
}

// Fills an image with deterministic random noise from Rng.
Image drawNoise() {
    const uint32 w = 256, h = 256;
    Image noise(fmt::gray8, w, h, ColorSpace::Gray);
    ImageView dv = noise.view();
    Rng rng(2026u);
    for (uint32 y = 0; y < h; ++y)
        for (uint32 x = 0; x < w; ++x)
            pixel::writeNorm(dv, (int32)x, (int32)y, 0, rng.nextFloat01());
    return noise;
}

} // namespace

int main(int argc, char** argv) {
    const std::string outDir = argc > 1 ? argv[1] : "";

    printScalarHelpers();
    printVectors();
    printMatrices();
    printRandom();

    try {
        const Image circle = drawCircle();
        circle.save(outPath(outDir, "02_circle.png"));
        std::cout << "Saved : " << outPath(outDir, "02_circle.png") << '\n';

        const Image noise = drawNoise();
        noise.save(outPath(outDir, "02_noise.png"));
        std::cout << "Saved : " << outPath(outDir, "02_noise.png") << '\n';
        return 0;
    } catch (const Error& e) {
        std::cerr << "ImageLib error: " << e.what() << '\n';
        return 1;
    }
}