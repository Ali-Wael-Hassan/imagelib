// tests/test_processing.cpp – Pixel, Color, Convolution, Filters, Resize,
// Transform, Morphology, Analysis
#include "tests.h"
#include "imagelib/processing/Pixel.h"
#include "imagelib/processing/Color.h"
#include "imagelib/processing/Convolution.h"
#include "imagelib/processing/Filters.h"
#include "imagelib/processing/Resize.h"
#include "imagelib/processing/Transform.h"
#include "imagelib/processing/Morphology.h"
#include "imagelib/processing/Analysis.h"
#include <vector>
#include <cmath>

using namespace iml;
using namespace iml::proc;

bool test_processing() {
    // ---------------- Pixel ----------------
    {
        Pixel p(0.2f, 0.3f, 0.4f, 0.5f);
        TEST_CHECK_NEAR(p.luminance(), 0.28596f, 1e-4f);
        Pixel sum = p + Pixel(1, 2, 3, 4);
        TEST_CHECK_NEAR(sum.r, 1.2f, 1e-5f);
        Pixel scaled = p * 2.f;
        TEST_CHECK_NEAR(scaled.g, 0.6f, 1e-6f);
        Pixel l = lerp(Pixel(0, 0, 0, 1), Pixel(1, 1, 1, 1), 0.5f);
        TEST_CHECK_NEAR(l.r, 0.5f, 1e-6f);
        Pixel8u p8(10, 20, 30);
        TEST_CHECK(p8.r == 10 && p8.a == 255);
        Pixel fl = clamp01(Pixel(-1.f, 2.f, 0.5f, 0.3f));
        TEST_CHECK(fl.r == 0.f && fl.g == 1.f && fl.b == 0.5f);
    }

    // ---------------- Color ----------------
    {
        TEST_CHECK_NEAR(color::srgbToLinear(0.f), 0.f, 1e-6f);
        TEST_CHECK_NEAR(color::linearToSrgb(1.f), 1.f, 1e-6f);
        TEST_CHECK_NEAR(color::linearToSrgb(color::srgbToLinear(0.3f)), 0.3f, 1e-4f);
        TEST_CHECK_NEAR(color::luminance(1, 1, 1), 1.f, 1e-5f);

        Pixel red = color::rgbToHsv(Pixel(1, 0, 0));
        TEST_CHECK_NEAR(red.r, 0.f, 1e-3f);
        TEST_CHECK_NEAR(red.g, 1.f, 1e-4f);
        TEST_CHECK_NEAR(red.b, 1.f, 1e-4f);
        Pixel c(0.1f, 0.4f, 0.9f, 1.f);
        Pixel rt = color::hsvToRgb(color::rgbToHsv(c));
        TEST_CHECK_NEAR(rt.r, c.r, 1e-3f);
        TEST_CHECK_NEAR(rt.g, c.g, 1e-3f);
        TEST_CHECK_NEAR(rt.b, c.b, 1e-3f);

        Pixel grayH = color::rgbToHsl(Pixel(0.5f, 0.5f, 0.5f));
        TEST_CHECK_NEAR(grayH.g, 0.f, 1e-4f);
        TEST_CHECK_NEAR(grayH.b, 0.5f, 1e-4f);
        Pixel rt2 = color::hslToRgb(color::rgbToHsl(c));
        TEST_CHECK_NEAR(rt2.r, c.r, 1e-3f);
        TEST_CHECK_NEAR(rt2.g, c.g, 1e-3f);
        TEST_CHECK_NEAR(rt2.b, c.b, 1e-3f);

        Pixel lab = color::rgbToLab(Pixel(1, 1, 1));
        TEST_CHECK_NEAR(lab.r, 100.f, 0.5f);
        TEST_CHECK_NEAR(lab.g, 0.f, 0.5f);
        TEST_CHECK_NEAR(lab.b, 0.f, 0.5f);
        Pixel rt3 = color::labToRgb(lab);
        TEST_CHECK_NEAR(rt3.r, 1.f, 1e-2f);
        TEST_CHECK_NEAR(rt3.g, 1.f, 1e-2f);
        TEST_CHECK_NEAR(rt3.b, 1.f, 1e-2f);

        Pixel pm = color::premultiply(Pixel(0.5f, 0.5f, 0.5f, 0.5f));
        TEST_CHECK_NEAR(pm.r, 0.25f, 1e-6f);
        Pixel up = color::unpremultiply(pm);
        TEST_CHECK_NEAR(up.r, 0.5f, 1e-6f);
        TEST_CHECK_NEAR(up.a, 0.5f, 1e-6f);

        Pixel over = color::blendOver(Pixel(0, 0, 0, 1), Pixel(1.f, 1.f, 1.f, 0.5f));
        TEST_CHECK_NEAR(over.r, 0.5f, 1e-6f);
        TEST_CHECK_NEAR(over.a, 1.f, 1e-6f);

        TEST_CHECK_NEAR(color::gammaEncode(0.25f, 2.f), 0.5f, 1e-4f); // 0.25^(1/2)

        // toGray: pure red -> luma ~ 54/255
        Image gr(fmt::gray8, 1, 1, ColorSpace::Gray);
        Image redI(fmt::rgb8, 1, 1);
        redI.set<uint8>(0, 0, 0, 255); redI.set<uint8>(0, 0, 1, 0); redI.set<uint8>(0, 0, 2, 0);
        color::toGray(redI.view(), gr.view());
        int g = gr.get<uint8>(0, 0, 0);
        TEST_CHECK(g >= 53 && g <= 56);
    }

    // ---------------- Convolution ----------------
    {
        Image src(fmt::gray8, 6, 6, ColorSpace::Gray);
        for (uint32 y = 0; y < 6; ++y)
            for (uint32 x = 0; x < 6; ++x)
                src.set<uint8>(x, y, 0, static_cast<uint8>(x * 40 + y * 5));
        Image dst(fmt::gray8, 6, 6, ColorSpace::Gray);
        conv::convolve(src.view(), dst.view(), conv::identityKernel());
        bool same = true;
        for (uint32 y = 0; y < 6; ++y)
            for (uint32 x = 0; x < 6; ++x)
                if (dst.get<uint8>(x, y, 0) != src.get<uint8>(x, y, 0)) same = false;
        TEST_CHECK(same);

        Image cst(fmt::gray8, 8, 8, ColorSpace::Gray);
        for (uint32 y = 0; y < 8; ++y) for (uint32 x = 0; x < 8; ++x) cst.set<uint8>(x, y, 0, 128);
        Image cstOut(fmt::gray8, 8, 8, ColorSpace::Gray);
        const conv::Kernel bk = conv::boxKernel(1);
        conv::boxBlur(cst.view(), cstOut.view(), 1);
        bool cst_same = true;
        for (uint32 y = 0; y < 8; ++y) for (uint32 x = 0; x < 8; ++x)
            if (cstOut.get<uint8>(x, y, 0) != 128) cst_same = false;
        TEST_CHECK(bk.width == 3 && bk.height == 3 && cst_same);

        conv::gaussianBlur(cst.view(), cstOut.view(), 1.0f);
        bool gs_same = true;
        for (uint32 y = 0; y < 8; ++y) for (uint32 x = 0; x < 8; ++x) {
            int v = cstOut.get<uint8>(x, y, 0);
            if (v < 126 || v > 130) gs_same = false;
        }
        TEST_CHECK(gs_same);

        TEST_CHECK_NEAR(conv::sum(conv::sobelXKernel()), 0.f, 1e-5f);
        TEST_CHECK_NEAR(conv::sum(conv::gaussianKernel(1.5f)), 1.f, 1e-3f);

        // sobel magnitude on a horizontal gradient vs constant image
        Image grad(fmt::gray32f, 16, 16, ColorSpace::Gray);
        for (uint32 y = 0; y < 16; ++y) for (uint32 x = 0; x < 16; ++x)
            grad.set<float>(x, y, 0, x * (16.f / 255.f));
        Image gx(fmt::gray32f, 16, 16, ColorSpace::Gray);
        Image gy(fmt::gray32f, 16, 16, ColorSpace::Gray);
        conv::convolve(grad.view(), gx.view(), conv::sobelXKernel());
        conv::convolve(grad.view(), gy.view(), conv::sobelYKernel());
        float g = std::sqrt(gx.get<float>(8, 8, 0) * gx.get<float>(8, 8, 0) +
                            gy.get<float>(8, 8, 0) * gy.get<float>(8, 8, 0));
        TEST_CHECK(g > 0.45f && g < 0.56f); // 4*(2*16/255) ~ 0.502

        Image zero(fmt::gray32f, 16, 16, ColorSpace::Gray);
        for (uint32 y = 0; y < 16; ++y) for (uint32 x = 0; x < 16; ++x) zero.set<float>(x, y, 0, 0.f);
        conv::convolve(zero.view(), gy.view(), conv::sobelYKernel());
        bool zok = true;
        for (uint32 y = 0; y < 16; ++y) for (uint32 x = 0; x < 16; ++x)
            if (gy.get<float>(x, y, 0) != 0.f) zok = false;
        TEST_CHECK(zok);
    }

    // ---------------- Filters ----------------
    {
        Image f1(fmt::gray32f, 2, 1, ColorSpace::Gray);
        f1.set<float>(0, 0, 0, 0.25f);
        f1.set<float>(1, 0, 0, 0.75f);
        Image out(fmt::gray32f, 2, 1, ColorSpace::Gray);
        filter::invert(f1.view(), out.view());
        TEST_CHECK_NEAR(out.get<float>(0, 0, 0), 0.75f, 1e-5f);
        TEST_CHECK_NEAR(out.get<float>(1, 0, 0), 0.25f, 1e-5f);

        filter::brightness(f1.view(), out.view(), 0.1f);
        TEST_CHECK_NEAR(out.get<float>(0, 0, 0), 0.35f, 1e-5f);
        filter::contrast(f1.view(), out.view(), 2.f);
        TEST_CHECK_NEAR(out.get<float>(0, 0, 0), 0.f, 1e-5f);
        TEST_CHECK_NEAR(out.get<float>(1, 0, 0), 1.f, 1e-5f);
        filter::threshold(f1.view(), out.view(), 0.5f);
        TEST_CHECK(out.get<float>(0, 0, 0) == 0.f);
        TEST_CHECK(out.get<float>(1, 0, 0) == 1.f);

        // median: isolated bright pixel surrounded by dark
        Image med(fmt::gray8, 3, 3, ColorSpace::Gray);
        for (uint32 y = 0; y < 3; ++y) for (uint32 x = 0; x < 3; ++x) med.set<uint8>(x, y, 0, 5);
        med.set<uint8>(1, 1, 0, 200);
        Image medOut(fmt::gray8, 3, 3, ColorSpace::Gray);
        filter::median(med.view(), medOut.view(), 1);
        TEST_CHECK(medOut.get<uint8>(1, 1, 0) == 5);
        TEST_CHECK(medOut.get<uint8>(0, 0, 0) == 5);
    }

    // ---------------- Resize ----------------
    {
        Image s2(fmt::gray8, 2, 2, ColorSpace::Gray);
        for (uint32 y = 0; y < 2; ++y) for (uint32 x = 0; x < 2; ++x)
            s2.set<uint8>(x, y, 0, static_cast<uint8>(x * 80 + y * 40));
        Image big(fmt::gray8, 4, 4, ColorSpace::Gray);
        resize::resize(s2.view(), big.view(), resize::Filter::Nearest);
        TEST_CHECK(big.get<uint8>(0, 0, 0) == 0);
        TEST_CHECK(big.get<uint8>(1, 0, 0) == 0);
        TEST_CHECK(big.get<uint8>(2, 0, 0) == 80); // nearest picks src col 1
        TEST_CHECK(big.get<uint8>(0, 2, 0) == 40);

        Image sf(fmt::gray32f, 2, 2, ColorSpace::Gray);
        sf.set<float>(0, 0, 0, 0.f);   sf.set<float>(1, 0, 0, 80.f);
        sf.set<float>(0, 1, 0, 160.f); sf.set<float>(1, 1, 0, 240.f);
        Image bf(fmt::gray32f, 4, 4, ColorSpace::Gray);
        resize::resize(sf.view(), bf.view(), resize::Filter::Bilinear);
        TEST_CHECK_NEAR(bf.get<float>(1, 1, 0), 60.f, 0.01f);
        TEST_CHECK_NEAR(bf.get<float>(2, 2, 0), 180.f, 0.01f);
    }

    // ---------------- Transform ----------------
    {
        // src 2 rows x 3 cols: {0,1,2} / {3,4,5}
        Image t(fmt::gray8, 3, 2, ColorSpace::Gray);
        int vals[6] = {0, 1, 2, 3, 4, 5};
        for (uint32 y = 0; y < 2; ++y) for (uint32 x = 0; x < 3; ++x)
            t.set<uint8>(x, y, 0, static_cast<uint8>(vals[y * 3 + x]));
        Image out(fmt::gray8, 3, 2, ColorSpace::Gray);

        transform::flipHorizontal(t.view(), out.view());
        TEST_CHECK(out.get<uint8>(0, 0, 0) == 2 && out.get<uint8>(2, 0, 0) == 0);
        TEST_CHECK(out.get<uint8>(0, 1, 0) == 5);

        transform::flipVertical(t.view(), out.view());
        TEST_CHECK(out.get<uint8>(0, 0, 0) == 3 && out.get<uint8>(0, 1, 0) == 0);

        transform::rotate180(t.view(), out.view());
        TEST_CHECK(out.get<uint8>(0, 0, 0) == 5 && out.get<uint8>(2, 1, 0) == 0);

        Image r90(fmt::gray8, 2, 3, ColorSpace::Gray);
        transform::rotate90(t.view(), r90.view());
        TEST_CHECK(r90.get<uint8>(0, 0, 0) == 3);
        TEST_CHECK(r90.get<uint8>(1, 0, 0) == 0);
        TEST_CHECK(r90.get<uint8>(0, 1, 0) == 4);
        TEST_CHECK(r90.get<uint8>(1, 2, 0) == 2);

        Image r270(fmt::gray8, 2, 3, ColorSpace::Gray);
        transform::rotate270(t.view(), r270.view());
        TEST_CHECK(r270.get<uint8>(0, 0, 0) == 2);
        TEST_CHECK(r270.get<uint8>(1, 0, 0) == 5);
        TEST_CHECK(r270.get<uint8>(0, 2, 0) == 0);

        Image tr(fmt::gray8, 2, 3, ColorSpace::Gray);
        transform::transpose(t.view(), tr.view());
        TEST_CHECK(tr.get<uint8>(0, 0, 0) == 0);
        TEST_CHECK(tr.get<uint8>(1, 0, 0) == 3);
        TEST_CHECK(tr.get<uint8>(0, 1, 0) == 1);
        TEST_CHECK(tr.get<uint8>(1, 2, 0) == 5);

        Image roi(fmt::gray8, 2, 2, ColorSpace::Gray);
        transform::copyRoi(t.view(), roi.view(), 1, 0, 2, 2);
        TEST_CHECK(roi.get<uint8>(0, 0, 0) == 1);
        TEST_CHECK(roi.get<uint8>(1, 1, 0) == 5);
    }

    // ---------------- Morphology ----------------
    {
        Image bin(fmt::gray8, 6, 6, ColorSpace::Gray);
        for (uint32 y = 0; y < 6; ++y) for (uint32 x = 0; x < 6; ++x) bin.set<uint8>(x, y, 0, 0);
        bin.set<uint8>(3, 3, 0, 255);
        Image dil(fmt::gray8, 6, 6, ColorSpace::Gray);
        Image ero(fmt::gray8, 6, 6, ColorSpace::Gray);
        morph::StructuringElement sq = morph::square(1);
        morph::dilate(bin.view(), dil.view(), sq);
        TEST_CHECK(dil.get<uint8>(3, 3, 0) == 255);
        TEST_CHECK(dil.get<uint8>(2, 2, 0) == 255);
        TEST_CHECK(dil.get<uint8>(4, 4, 0) == 255);
        TEST_CHECK(dil.get<uint8>(0, 0, 0) == 0);

        morph::erode(dil.view(), ero.view(), sq);
        TEST_CHECK(ero.get<uint8>(3, 3, 0) == 255);
        TEST_CHECK(ero.get<uint8>(2, 2, 0) == 0); // border of the 3x3 block
        TEST_CHECK(ero.get<uint8>(4, 4, 0) == 0);

        Image op(fmt::gray8, 6, 6, ColorSpace::Gray);
        morph::open(bin.view(), op.view(), sq); // isolated pixel removed
        TEST_CHECK(op.get<uint8>(3, 3, 0) == 0);

        morph::StructuringElement cr = morph::cross(1);
        TEST_CHECK(cr.size() == 5);
        Image cl(fmt::gray8, 6, 6, ColorSpace::Gray);
        morph::close(bin.view(), cl.view(), sq);
        TEST_CHECK(cl.get<uint8>(3, 3, 0) == 255);
    }

    // ---------------- Analysis ----------------
    {
        Image ramp(fmt::gray8, 3, 3, ColorSpace::Gray);
        for (uint32 i = 0; i < 9; ++i) ramp.set<uint8>(i % 3, i / 3, 0, static_cast<uint8>(i));
        float mn, mx, mean = 0.f, sdev = 0.f;
        analysis::minMax(ramp.view(), 0, mn, mx);
        TEST_CHECK_NEAR(mn, 0.f, 1e-6f);
        TEST_CHECK_NEAR(mx, 8.f / 255.f, 1e-6f);
        analysis::meanStdDev(ramp.view(), 0, mean, sdev);
        TEST_CHECK_NEAR(mean, 4.f / 255.f, 1e-6f);
        TEST_CHECK(sdev > 0.f && sdev < 0.03f);

        std::vector<uint64> h = analysis::histogram(ramp.view(), 0, 256);
        uint64 total = 0;
        for (uint64 v : h) total += v;
        TEST_CHECK(total == 9);
        TEST_CHECK(h[0] == 1 && h[8] == 1);

        Image bi(fmt::gray8, 100, 100, ColorSpace::Gray);
        for (uint32 y = 0; y < 100; ++y)
            for (uint32 x = 0; x < 100; ++x)
                bi.set<uint8>(x, y, 0, x < 50 ? 25 : 225);
        float t = analysis::otsuThreshold(bi.view());
        TEST_CHECK(t > 0.08f && t < 0.12f);

        float ent = analysis::entropyBits(bi.view());
        TEST_CHECK(ent > 0.9f && ent <= 1.01f); // balanced two-tone => ~1 bit
    }

    // ---------------- Parallel paths behave like the scalar paths ----------------
    // Every kernel with an ExecutionPolicy overload must produce the same bytes
    // as its serial twin. Exercise the SIMD + multithreaded path (and the
    // Auto fallback) on a frame large enough to actually split rows.
    {
        const uint32 W = 400, H = 300;
        Image ps(fmt::rgb8, W, H, ColorSpace::SRGB);
        for (uint32 y = 0; y < H; ++y)
            for (uint32 x = 0; x < W; ++x) {
                ps.set<uint8>(x, y, 0, static_cast<uint8>((x * 3 + y * 5) & 255));
                ps.set<uint8>(x, y, 1, static_cast<uint8>((x * 7 + y * 11) & 255));
                ps.set<uint8>(x, y, 2, static_cast<uint8>((x * 13 + y * 17) & 255));
            }
        auto compare = [&](const Image& a, const Image& b) {
            bool same = a.width() == b.width() && a.height() == b.height()
                     && a.channels() == b.channels();
            const uint32 w = a.width(), chh = a.channels();
            for (uint32 y = 0; same && y < a.height(); ++y)
                for (uint32 x = 0; same && x < w; ++x)
                    for (uint32 c = 0; c < chh; ++c)
                        if (a.get<uint8>(x, y, c) != b.get<uint8>(x, y, c)) { same = false; break; }
            return same;
        };

        Image s(fmt::rgb8, W, H, ColorSpace::SRGB), p(fmt::rgb8, W, H, ColorSpace::SRGB);
        const ExecutionPolicy par = ExecutionPolicy::simdParallel();

        // toGray
        Image sg(fmt::gray8, W, H, ColorSpace::Gray), pg(fmt::gray8, W, H, ColorSpace::Gray);
        color::toGray(ps.view(), sg.view());
        color::toGray(ps.view(), pg.view(), par);
        TEST_CHECK(compare(sg, pg));

        // invert
        filter::invert(ps.view(), s.view());
        filter::invert(ps.view(), p.view(), par);
        TEST_CHECK(compare(s, p));

        // brightness / contrast / linear
        filter::brightness(ps.view(), s.view(), -0.3f);
        filter::brightness(ps.view(), p.view(), -0.3f, par);
        TEST_CHECK(compare(s, p));
        filter::contrast(ps.view(), s.view(), 1.5f);
        filter::contrast(ps.view(), p.view(), 1.5f, par);
        TEST_CHECK(compare(s, p));
        filter::linear(ps.view(), s.view(), 0.8f, 0.1f);
        filter::linear(ps.view(), p.view(), 0.8f, 0.1f, par);
        TEST_CHECK(compare(s, p));
        filter::threshold(ps.view(), s.view(), 0.5f);
        filter::threshold(ps.view(), p.view(), 0.5f, par);
        TEST_CHECK(compare(s, p));

        // flips (policy-overloaded)
        transform::flipHorizontal(ps.view(), s.view());
        transform::flipHorizontal(ps.view(), p.view(), par);
        TEST_CHECK(compare(s, p));
        transform::flipVertical(ps.view(), s.view());
        transform::flipVertical(ps.view(), p.view(), par);
        TEST_CHECK(compare(s, p));
        transform::rotate180(ps.view(), s.view());
        transform::rotate180(ps.view(), p.view(), par);
        TEST_CHECK(compare(s, p));

        // ROI copy
        Image roi(fmt::rgb8, 120, 90, ColorSpace::SRGB), roiP(fmt::rgb8, 120, 90, ColorSpace::SRGB);
        transform::copyRoi(ps.view(), roi.view(), 40, 50, 120, 90);
        transform::copyRoi(ps.view(), roiP.view(), 40, 50, 120, 90, par);
        TEST_CHECK(compare(roi, roiP));

        // convolution + separable blur + resize on a gray source
        Image gs(fmt::gray8, W, H, ColorSpace::Gray);
        Image out(fmt::gray8, W, H, ColorSpace::Gray), outP(fmt::gray8, W, H, ColorSpace::Gray);
        for (uint32 y = 0; y < H; ++y)
            for (uint32 x = 0; x < W; ++x)
                gs.set<uint8>(x, y, 0, static_cast<uint8>((x * 3 + y * 29) & 255));
        conv::convolve(gs.view(), out.view(), conv::sobelXKernel(), conv::BorderMode::Clamp);
        conv::convolve(gs.view(), outP.view(), conv::sobelXKernel(), conv::BorderMode::Clamp, par);
        TEST_CHECK(compare(out, outP));

        conv::gaussianBlur(gs.view(), out.view(), 1.5f, conv::BorderMode::Clamp, ExecutionPolicy::serial());
        conv::gaussianBlur(gs.view(), outP.view(), 1.5f, conv::BorderMode::Clamp, par);
        TEST_CHECK(compare(out, outP));

        conv::boxBlur(gs.view(), out.view(), 2, conv::BorderMode::Clamp);
        conv::boxBlur(gs.view(), outP.view(), 2, conv::BorderMode::Clamp, par);
        TEST_CHECK(compare(out, outP));

        Image upS(fmt::gray8, 800, 600, ColorSpace::Gray), upP(fmt::gray8, 800, 600, ColorSpace::Gray);
        resize::resize(gs.view(), upS.view(), resize::Filter::Bilinear, conv::BorderMode::Clamp);
        resize::resize(gs.view(), upP.view(), resize::Filter::Bilinear, conv::BorderMode::Clamp, par);
        TEST_CHECK(compare(upS, upP));

        resize::resize(gs.view(), upS.view(), resize::Filter::Nearest, conv::BorderMode::Clamp);
        resize::resize(gs.view(), upP.view(), resize::Filter::Nearest, conv::BorderMode::Clamp, par);
        TEST_CHECK(compare(upS, upP));
    }

    return true;
}