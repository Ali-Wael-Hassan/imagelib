// tests/test_codecs.cpp – Codec layer roundtrips (PNG/JPEG/BMP/TGA)
#include "tests.h"
#include "imagelib/core/Image.h"
#include "imagelib/codecs/Codec.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace iml;
using namespace iml::codecs;

static int nearI(int a, int b, int tol) { int d = a - b; return d < 0 ? -d : d; }

bool test_codecs() {
    // --- helpers: detectFormat / extensionOf -------------------------------
    {
        TEST_CHECK(extensionOf("foo.png") == "png");
        TEST_CHECK(extensionOf("/path/to/x.PNG") == "png");
        TEST_CHECK(extensionOf("dir.TGA") == "tga");
        TEST_CHECK(extensionOf("noext") == "");
    }
    {
        // PNG magic bytes
        const byte pngSig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
        TEST_CHECK(detectFormat(pngSig, 8) == ImageCodecFormat::PNG);
        // JPEG magic
        const byte jpgSig[3] = {0xFF, 0xD8, 0xFF};
        TEST_CHECK(detectFormat(jpgSig, 3) == ImageCodecFormat::JPEG);
        // BMP magic
        const byte bmpSig[2] = {'B', 'M'};
        TEST_CHECK(detectFormat(bmpSig, 2) == ImageCodecFormat::BMP);
        // Unknown
        const byte nothing[8] = {0};
        TEST_CHECK(detectFormat(nothing, 8) == ImageCodecFormat::Unknown);
    }

    // --- build a distinctive test image --------------------------------------
    Image src(fmt::rgb8, 32, 24, ColorSpace::SRGB);
    for (uint32 y = 0; y < 24; ++y)
        for (uint32 x = 0; x < 32; ++x) {
            src.set<uint8_t>(x, y, 0, static_cast<uint8_t>(x * 8));
            src.set<uint8_t>(x, y, 1, static_cast<uint8_t>(y * 10));
            src.set<uint8_t>(x, y, 2, static_cast<uint8_t>((x + y) * 3));
        }

    // --- PNG roundtrip (lossless) -------------------------------------------
    {
        const char* path = "codec_roundtrip.png";
        saveImage(src, path, SaveOptions{});
        Image back(path);
        TEST_CHECK(back.width() == 32 && back.height() == 24);
        TEST_CHECK(back.channels() == 3);
        for (int i = 0; i < 32 * 24; ++i) {
            uint32 x = static_cast<uint32>(i % 32), y = static_cast<uint32>(i / 32);
            TEST_CHECK(back.get<uint8_t>(x, y, 0) == src.get<uint8_t>(x, y, 0));
            TEST_CHECK(back.get<uint8_t>(x, y, 1) == src.get<uint8_t>(x, y, 1));
        }
        std::remove(path);
    }

    // --- BMP roundtrip (24-bit forced) ---------------------------------------
    {
        const char* path = "codec_roundtrip.bmp";
        saveImage(src, path, SaveOptions{});
        Image back;
        loadImage(back, path);
        TEST_CHECK(back.width() == 32 && back.height() == 24);
        TEST_CHECK(back.channels() == 3);
        TEST_CHECK(back.get<uint8_t>(0, 0, 0) == 0);
        TEST_CHECK(back.get<uint8_t>(3, 2, 1) == src.get<uint8_t>(3, 2, 1));
        std::remove(path);
    }

    // --- JPEG roundtrip (lossy, approximate) --------------------------------
    {
        const char* path = "codec_roundtrip.jpg";
        SaveOptions opt;
        opt.jpegQuality = 92;
        saveImage(src, path, opt);
        Image back;
        loadImage(back, path);
        TEST_CHECK(back.width() == 32 && back.height() == 24);
        TEST_CHECK(back.channels() == 3);
        int worst = 0;
        for (uint32 y = 0; y < 24; ++y)
            for (uint32 x = 0; x < 32; ++x) {
                int d = nearI(back.get<uint8_t>(x, y, 0), src.get<uint8_t>(x, y, 0), 255);
                if (d > worst) worst = d;
            }
        TEST_CHECK(worst <= 24); // JPEG error tolerance
        std::remove(path);
    }

    // --- TGA roundtrip -------------------------------------------------------
    {
        const char* path = "codec_roundtrip.tga";
        saveImage(src, path, SaveOptions{});
        Image back;
        loadImage(back, path);
        TEST_CHECK(back.width() == 32 && back.height() == 24);
        TEST_CHECK(back.channels() >= 3);
        std::remove(path);
    }

    // --- supportsRead / supportsWrite / registry ----------------------------
    {
        TEST_CHECK(supportsRead("a.png"));
        TEST_CHECK(supportsWrite("a.jpg"));
        TEST_CHECK(supportsWrite("a.bmp"));
        TEST_CHECK(!supportsRead("a.xyz"));
        TEST_CHECK(codecs::CodecRegistry::instance().count() >= 4); // png/jpg/bmp/tga
    }

    // --- wrong extension fails loudly ----------------------------------------
    {
        bool threw = false;
        try { saveImage(src, "codec_roundtrip_wrong.xyz", SaveOptions{}); }
        catch (const iml::UnsupportedFormatError&) { threw = true; }
        TEST_CHECK(threw);
    }

    // --- large image roundtrip exercises the parallel row codec ---------------
    {
        // 512x384 > the 16-row parallel threshold: decode copy and encode
        // packing both split rows across the thread pool.
        Image big(fmt::rgba8, 512, 384, ColorSpace::SRGB);
        for (uint32 y = 0; y < 384; ++y)
            for (uint32 x = 0; x < 512; ++x) {
                big.set<uint8>(x, y, 0, static_cast<uint8>((x * 31 + y * 7) & 255));
                big.set<uint8>(x, y, 1, static_cast<uint8>((x * 11 + y * 53) & 255));
                big.set<uint8>(x, y, 2, static_cast<uint8>((x * 3 + y * 13) & 255));
                big.set<uint8>(x, y, 3, static_cast<uint8>(x < 256 ? 80 : 200));
            }
        const char* path = "codec_parallel_roundtrip.png";
        saveImage(big, path, SaveOptions{});
        Image back;
        loadImage(back, path);
        TEST_CHECK(back.width() == 512 && back.height() == 384 && back.channels() == 4);
        bool ok = true;
        for (uint32 y = 0; y < 384; y += 17)
            for (uint32 x = 0; x < 512; x += 13) {
                for (uint32 c = 0; c < 4; ++c)
                    if (back.get<uint8>(x, y, c) != big.get<uint8>(x, y, c)) ok = false;
            }
        TEST_CHECK(ok);
        std::remove(path);
    }

    return true;
}