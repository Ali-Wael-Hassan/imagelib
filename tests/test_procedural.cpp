// tests/test_procedural.cpp – Noise, Masks, Heightmaps, Textures
#include "tests.h"
#include "imagelib/procedural/Noise.h"
#include "imagelib/procedural/Masks.h"
#include "imagelib/procedural/Heightmaps.h"
#include "imagelib/procedural/Textures.h"
#include <cmath>

using namespace iml;
using namespace iml::procgen;

template <typename Fn>
static float rangeMin(const Image& img, uint32 c, Fn fn) {
    float mn = 1e30f, mx = -1e30f;
    for (uint32 y = 0; y < img.height(); ++y)
        for (uint32 x = 0; x < img.width(); ++x) {
            float v = fn(img.get<float>(x, y, c));
            if (v < mn) mn = v;
            if (v > mx) mx = v;
        }
    return mx - mn;
}

static float imgMean(const Image& img, uint32 c) {
    double s = 0.0;
    for (uint32 y = 0; y < img.height(); ++y)
        for (uint32 x = 0; x < img.width(); ++x)
            s += img.get<float>(x, y, c);
    return static_cast<float>(s / (img.width() * img.height()));
}

static bool inUnit(const Image& img) {
    for (uint32 y = 0; y < img.height(); ++y)
        for (uint32 x = 0; x < img.width(); ++x)
            for (uint32 c = 0; c < img.channels(); ++c)
                if (img.get<float>(x, y, c) < -1e-4f || img.get<float>(x, y, c) > 1.f + 1e-4f)
                    return false;
    return true;
}

bool test_procedural() {
    // ---------------- Noise ----------------
    {
        Image wh(fmt::gray32f, 64, 64, ColorSpace::Gray);
        fillNoise(wh.view(), NoiseType::White, 42, 2.f);
        TEST_CHECK(inUnit(wh));
        TEST_CHECK(rangeMin(wh, 0, [](float v){ return v; }) > 0.5f);

        fillNoise(wh.view(), NoiseType::Perlin, 42, 4.f);
        TEST_CHECK(inUnit(wh));
        TEST_CHECK_NEAR(imgMean(wh, 0), 0.5f, 0.08f);

        Image sim(fmt::gray32f, 32, 32, ColorSpace::Gray);
        fillNoise(sim.view(), NoiseType::Simplex, 7, 2.f);
        Image sim2(fmt::gray32f, 32, 32, ColorSpace::Gray);
        fillNoise(sim2.view(), NoiseType::Simplex, 7, 2.f);
        bool eq = true;
        for (uint32 y = 0; y < 32 && eq; ++y) for (uint32 x = 0; x < 32 && eq; ++x)
            if (sim.get<float>(x, y, 0) != sim2.get<float>(x, y, 0)) eq = false;
        TEST_CHECK(eq); // deterministic under same seed

        fillNoise(sim.view(), NoiseType::Worley, 3, 1.f);
        TEST_CHECK(inUnit(sim));
        TEST_CHECK(rangeMin(sim, 0, [](float v){ return v; }) > 1e-3f);

        fillNoise(sim.view(), NoiseType::Fbm, 3, 2.f);
        TEST_CHECK(inUnit(sim));
        fillNoise(sim.view(), NoiseType::Ridged, 3, 2.f);
        TEST_CHECK(inUnit(sim));
        fillNoise(sim.view(), NoiseType::DomainWarp, 3, 2.f);
        TEST_CHECK(inUnit(sim));

        // 8-bit route via NoiseSettings struct
        Image g8(fmt::gray8, 16, 16, ColorSpace::Gray);
        NoiseSettings ns; ns.type = NoiseType::Value; ns.seed = 1; ns.frequency = 3.f;
        fillNoise(g8.view(), ns);
        TEST_CHECK(g8.get<uint8>(0, 0, 0) != g8.get<uint8>(15, 15, 0));
    }

    // ---------------- Noise: parallel policy matches serial ----------------
    {
        Image a(fmt::gray32f, 120, 90, ColorSpace::Gray);
        Image b(fmt::gray32f, 120, 90, ColorSpace::Gray);
        NoiseSettings ns; ns.type = NoiseType::Perlin; ns.seed = 99; ns.frequency = 3.f;
        fillNoise(a.view(), ns);
        fillNoise(b.view(), ns, ExecutionPolicy::simdParallel());
        bool eq = true;
        for (uint32 y = 0; y < 90 && eq; ++y)
            for (uint32 x = 0; x < 120 && eq; ++x)
                if (a.get<float>(x, y, 0) != b.get<float>(x, y, 0)) eq = false;
        TEST_CHECK(eq);
    }

    // ---------------- Masks ----------------
    {
        Image gr(fmt::gray32f, 8, 8, ColorSpace::Gray);
        fillGradient(gr.view(), GradientAxis::Horizontal, 0.2f, 0.8f);
        TEST_CHECK_NEAR(gr.get<float>(0, 4, 0), 0.2f, 1e-5f);
        TEST_CHECK_NEAR(gr.get<float>(7, 4, 0), 0.8f, 1e-5f);
        TEST_CHECK_NEAR(gr.get<float>(3, 4, 0), 0.2f + 0.6f * (3.f / 7.f), 1e-5f);

        fillGradient(gr.view(), GradientAxis::Vertical, 0.f, 1.f);
        TEST_CHECK_NEAR(gr.get<float>(4, 0, 0), 0.f, 1e-5f);
        TEST_CHECK_NEAR(gr.get<float>(4, 7, 0), 1.f, 1e-5f);

        fillGradient(gr.view(), GradientAxis::Radial, 0.f, 1.f);
        TEST_CHECK_NEAR(gr.get<float>(3, 3, 0), 1.f / 7.f, 1e-5f); // nearest pixel to center (3.5,3.5)
        TEST_CHECK(gr.get<float>(0, 0, 0) > 0.95f);               // corner ~1

        Image cm(fmt::gray32f, 16, 16, ColorSpace::Gray);
        fillCircleMask(cm.view(), 8.f, 8.f, 4.f, 0.f);
        TEST_CHECK_NEAR(cm.get<float>(8, 8, 0), 1.f, 1e-5f);
        TEST_CHECK_NEAR(cm.get<float>(0, 0, 0), 0.f, 1e-5f);
        TEST_CHECK_NEAR(cm.get<float>(8, 5, 0), 1.f, 1e-5f); // strictly inside radius 4

        fillRingMask(cm.view(), 8.f, 8.f, 2.f, 4.f, 0.f);
        TEST_CHECK_NEAR(cm.get<float>(8, 8, 0), 0.f, 1e-5f); // hole
        TEST_CHECK_NEAR(cm.get<float>(8, 5, 0), 1.f, 1e-5f); // band (dist 3)
        TEST_CHECK_NEAR(cm.get<float>(0, 0, 0), 0.f, 1e-5f); // outside

        Image cb(fmt::gray32f, 4, 4, ColorSpace::Gray);
        fillCheckerboard(cb.view(), 2, 2, 0.1f, 0.9f);
        TEST_CHECK_NEAR(cb.get<float>(0, 0, 0), 0.1f, 1e-5f);
        TEST_CHECK_NEAR(cb.get<float>(2, 0, 0), 0.9f, 1e-5f);
        TEST_CHECK_NEAR(cb.get<float>(0, 2, 0), 0.9f, 1e-5f);
        TEST_CHECK_NEAR(cb.get<float>(3, 3, 0), 0.1f, 1e-5f);
    }

    // ---------------- Heightmaps ----------------
    {
        Image flat(fmt::gray32f, 16, 16, ColorSpace::Gray);
        for (uint32 y = 0; y < 16; ++y) for (uint32 x = 0; x < 16; ++x) flat.set<float>(x, y, 0, 0.5f);
        Image nm(fmt::rgb32f, 16, 16, ColorSpace::SRGB);
        heightmapNormal(flat.view(), nm.view(), 2.f);
        TEST_CHECK_NEAR(nm.get<float>(8, 8, 2), 1.f, 1e-5f);  // nz
        TEST_CHECK_NEAR(nm.get<float>(8, 8, 0), 0.5f, 1e-5f); // nx
        TEST_CHECK_NEAR(nm.get<float>(8, 8, 1), 0.5f, 1e-5f); // ny

        Image ramp(fmt::gray32f, 16, 16, ColorSpace::Gray);
        for (uint32 y = 0; y < 16; ++y) for (uint32 x = 0; x < 16; ++x)
            ramp.set<float>(x, y, 0, x * (1.f / 16.f));
        heightmapNormal(ramp.view(), nm.view(), 1.f);
        TEST_CHECK(nm.get<float>(8, 8, 1) > 0.49f && nm.get<float>(8, 8, 1) < 0.51f);
        TEST_CHECK(nm.get<float>(8, 8, 0) != 0.5f);
        TEST_CHECK(nm.get<float>(8, 8, 2) > 0.9f);

        Image sh(fmt::gray32f, 16, 16, ColorSpace::Gray);
        heightmapShade(flat.view(), sh.view(), 0.f, 90.f, 0.3f); // light straight down
        TEST_CHECK_NEAR(sh.get<float>(8, 8, 0), 1.f, 1e-5f);

        Image co(fmt::gray32f, 16, 16, ColorSpace::Gray);
        heightmapContours(ramp.view(), co.view(), 0.25f, 0.02f);
        TEST_CHECK_NEAR(co.get<float>(0, 8, 0), 0.f, 1e-5f);  // level 0 -> line
        TEST_CHECK_NEAR(co.get<float>(8, 8, 0), 0.f, 1e-5f);  // level 0.5 -> line
        TEST_CHECK_NEAR(co.get<float>(5, 8, 0), 5.f / 16.f, 1e-5f); // between contour lines
    }

    // ---------------- Textures ----------------
    {
        Image wood(fmt::rgb32f, 64, 64, ColorSpace::SRGB);
        fillWood(wood.view(), 11, 3.f, 1.f);
        TEST_CHECK(inUnit(wood));
        TEST_CHECK(rangeMin(wood, 0, [](float v){ return v; }) > 0.2f);
        TEST_CHECK(rangeMin(wood, 1, [](float v){ return v; }) > 0.2f);

        Image wood2(fmt::rgb32f, 64, 64, ColorSpace::SRGB);
        fillWood(wood2.view(), 11, 3.f, 1.f);
        TEST_CHECK(wood.get<float>(20, 20, 0) == wood2.get<float>(20, 20, 0));

        Image marble(fmt::rgb32f, 64, 64, ColorSpace::SRGB);
        fillMarble(marble.view(), 5, 4.f, 2.f);
        TEST_CHECK(inUnit(marble));
        TEST_CHECK(rangeMin(marble, 0, [](float v){ return v; }) > 0.3f);

        Image clouds(fmt::rgb32f, 64, 64, ColorSpace::SRGB);
        fillClouds(clouds.view(), 9, 0.06f, 4);
        TEST_CHECK(inUnit(clouds));
        TEST_CHECK(rangeMin(clouds, 0, [](float v){ return v; }) > 0.3f);

        Image plasma(fmt::rgb32f, 64, 64, ColorSpace::SRGB);
        fillPlasma(plasma.view(), 4, 2.f, 3);
        TEST_CHECK(inUnit(plasma));
        bool colorful = false;
        for (uint32 y = 0; y < 64 && !colorful; ++y)
            for (uint32 x = 0; x < 64 && !colorful; ++x) {
                float r = plasma.get<float>(x, y, 0);
                float g = plasma.get<float>(x, y, 1);
                float b = plasma.get<float>(x, y, 2);
                if (std::fabs(r - g) > 0.25f || std::fabs(g - b) > 0.25f) colorful = true;
            }
        TEST_CHECK(colorful);

        // 8-bit texture route stays in bounds
        Image w8(fmt::rgb8, 32, 32, ColorSpace::SRGB);
        fillWood(w8.view(), 1, 3.f, 1.f);
        TEST_CHECK(w8.get<uint8>(0, 0, 0) >= 0 && w8.get<uint8>(31, 31, 2) <= 255);
    }

    return true;
}