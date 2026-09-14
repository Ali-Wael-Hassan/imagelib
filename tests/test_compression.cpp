// tests/test_compression.cpp – BitPacking, PackedPixels, Quantization, PackedImage
#include "tests.h"
#include "imagelib/core/Image.h"
#include "imagelib/compression/BitPacking.h"
#include "imagelib/compression/PackedPixels.h"
#include "imagelib/compression/Quantization.h"
#include "imagelib/compression/PackedImage.h"
#include <cstring>
#include <cmath>

using namespace iml;
using namespace iml::comp;

static int nearV(int a, int b, int tol) {
    int d = a - b;
    return d < 0 ? -d : d;
}

bool test_compression() {
    // --- BitWriter / BitReader roundtrip ---
    {
        BitWriter w;
        w.writeBits(0x0F, 4);
        w.writeBits(0x00, 4);
        w.writeBits(0xAAu, 8);
        w.writeBits(0x05, 3);
        w.writeBits(0xDEADBEEFu, 32);
        w.writeBits(0x02, 2);
        w.alignToByte();
        w.writeBytes("AB", 2);
        mem::Buffer b = w.finish();
        BitReader r(b.data(), b.size());
        TEST_CHECK(r.readBits(4) == 0x0F);
        TEST_CHECK(r.readBits(4) == 0x00);
        TEST_CHECK(r.readBits(8) == 0xAA);
        TEST_CHECK(r.readBits(3) == 0x05);
        TEST_CHECK(r.readBits(32) == 0xDEADBEEF);
        TEST_CHECK(r.readBits(2) == 0x02);
        r.alignToByte();
        char s[3] = {0};
        r.readBytes(s, 2);
        TEST_CHECK(std::strcmp(s, "AB") == 0);
        TEST_CHECK(r.eof());
    }
    {
        // 53 payload + 3 pad + 16 bytes = 72 bits
        BitWriter w2;
        w2.writeBits(0x0F, 4); w2.writeBits(0x00, 4); w2.writeBits(0xAAu, 8);
        w2.writeBits(0x05, 3); w2.writeBits(0xDEADBEEFu, 32); w2.writeBits(0x02, 2);
        w2.alignToByte(); w2.writeBytes("AB", 2);
        TEST_CHECK(w2.bitsWritten() == 72);
    }
    TEST_CHECK(bitWidth(0) == 0);
    TEST_CHECK(bitWidth(1) == 1);
    TEST_CHECK(bitWidth(255) == 8);
    TEST_CHECK(bitWidth(256) == 9);
    TEST_CHECK(reverseBits(0b10100u, 5) == 0b00101u);
    TEST_CHECK(bytesForBits(1) == 1);
    TEST_CHECK(bytesForBits(8) == 1);
    TEST_CHECK(bytesForBits(9) == 2);

    // --- PackedPixels value conversions ---
    {
        uint8 r = 0, g = 0, b = 0, a = 0;
        TEST_CHECK(packPixelRGBA(PackedFormat::RGB332, 255, 0, 0, 255) == (uint64_t)0b11100000);
        unpackPixelRGBA(PackedFormat::RGB332, 0b11100000u, r, g, b, a);
        TEST_CHECK(r == 255 && g == 0 && b == 0 && a == 255);

        uint64 v565 = packPixelRGBA(PackedFormat::RGB565, 200, 100, 50, 255);
        TEST_CHECK(v565 == (((uint64_t)(200 >> 3) << 11) | ((uint64_t)(100 >> 2) << 5) | (uint64_t)(50 >> 3)));
        unpackPixelRGBA(PackedFormat::RGB565, v565, r, g, b, a);
        TEST_CHECK(nearV(r, 200, 8) <= 8 && nearV(g, 100, 4) <= 4 && nearV(b, 50, 8) <= 8);

        uint64 bgr = packPixelRGBA(PackedFormat::BGR565, 200, 100, 50, 255);
        TEST_CHECK(bgr != v565);
        unpackPixelRGBA(PackedFormat::BGR565, bgr, r, g, b, a);
        TEST_CHECK(nearV(r, 200, 8) <= 8 && nearV(g, 100, 4) <= 4 && nearV(b, 50, 8) <= 8);

        uint64 v4444 = packPixelRGBA(PackedFormat::RGBA4444, 17, 34, 68, 136);
        unpackPixelRGBA(PackedFormat::RGBA4444, v4444, r, g, b, a);
        TEST_CHECK(nearV(r, 17, 16) <= 16 && nearV(g, 34, 16) <= 16 && nearV(b, 68, 16) <= 16 && nearV(a, 136, 16) <= 16);

        unpackPixelRGBA(PackedFormat::Gray4, 0b1010u, r, g, b, a);
        TEST_CHECK(r == ((uint8_t)(10 * 255 / 15)) && r == g && r == b && a == 255);

        uint64 v5551 = packPixelRGBA(PackedFormat::RGBA5551, 255, 0, 0, 255);
        TEST_CHECK(v5551 == (((uint64_t)0b11111 << 11) | 1));
        unpackPixelRGBA(PackedFormat::RGBA5551, v5551, r, g, b, a);
        TEST_CHECK(r == 255 && a == 255);
    }

    // --- sub-byte scanline access ---
    {
        TEST_CHECK(packedRowBytes(10, PackedFormat::Gray1) == 4); // 10px @1bpp = 2 bytes, aligned 4
        TEST_CHECK(packedRowBytes(7, PackedFormat::Gray4) == 4);  // 28 bits = 4 bytes
        byte row[8] = {0};
        const uint32 w = 10;
        uint64 vals[10] = {0};
        for (uint32 x = 0; x < w; ++x) { vals[x] = x % 2; writePackedPixel(row, x, PackedFormat::Gray1, vals[x]); }
        for (uint32 x = 0; x < w; ++x) TEST_CHECK(readPackedPixel(row, x, PackedFormat::Gray1) == vals[x]);
        TEST_CHECK(row[0] == 0x55); // pixel0=0 at top bit -> 0101 0101
    }

    // --- Gray16 pack/unpack ---
    {
        byte row[8] = {0};
        writePackedPixel(row, 0, PackedFormat::Gray16, 0x1234);
        writePackedPixel(row, 1, PackedFormat::Gray16, 0xABCD);
        TEST_CHECK(row[0] == 0x12 && row[1] == 0x34);
        TEST_CHECK(row[2] == 0xAB && row[3] == 0xCD);
        TEST_CHECK(readPackedPixel(row, 0, PackedFormat::Gray16) == 0x1234);
        TEST_CHECK(readPackedPixel(row, 1, PackedFormat::Gray16) == 0xABCD);
    }

    // --- Quantization ---
    {
        TEST_CHECK(quantizeU8(255, 8) == 7 && quantizeU8(0, 8) == 0);
        TEST_CHECK(dequantizeU8(7, 8) == 255);
        TEST_CHECK(dequantizeU8(quantizeU8(200, 8), 8) <= 255);

        Image src(ImageFormat(PixelFormat::RGB, DataType::UInt8), 64, 64, ColorSpace::SRGB);
        Image dst(ImageFormat(PixelFormat::RGB, DataType::UInt8), 64, 64, ColorSpace::SRGB);
        for (uint32 y = 0; y < 64; ++y)
            for (uint32 x = 0; x < 64; ++x) {
                src.set<uint8_t>(x, y, 0, static_cast<uint8_t>(x * 4));
                src.set<uint8_t>(x, y, 1, static_cast<uint8_t>(y * 4));
                src.set<uint8_t>(x, y, 2, static_cast<uint8_t>(128));
            }
        QuantizeParams p;
        p.bitsR = 5; p.bitsG = 6; p.bitsB = 5; p.dither = DitherMode::None;
        quantizeImage(src.view(), dst.view(), p);
        for (uint32 y = 0; y < 64; ++y)
            for (uint32 x = 0; x < 64; ++x) {
                uint8_t rr = dst.get<uint8_t>(x, y, 0);
                TEST_CHECK(nearV(rr, src.get<uint8_t>(x, y, 0), 8) <= 8);
            }

        QuantizeParams pd;
        pd.bitsR = 5; pd.bitsG = 6; pd.bitsB = 5; pd.dither = DitherMode::FloydSteinberg;
        Image dither = quantize(src, pd);
        TEST_CHECK(dither.width() == 64 && dither.channels() == 3);

        Image gray(ImageFormat(PixelFormat::Gray, DataType::UInt8), 32, 32, ColorSpace::Gray);
        for (uint32 y = 0; y < 32; ++y)
            for (uint32 x = 0; x < 32; ++x)
                gray.set<uint8_t>(x, y, 0, static_cast<uint8_t>(x * 8));
        QuantizeParams pg;
        pg.bitsGray = 4;
        Image gq = quantize(gray, pg);
        TEST_CHECK(gq.get<uint8_t>(7, 0, 0) % 17 == 0);

        // Parallel (row-split) quantization must match the serial result.
        Image big(fmt::rgb8, 200, 120, ColorSpace::SRGB);
        for (uint32 y = 0; y < 120; ++y)
            for (uint32 x = 0; x < 200; ++x) {
                big.set<uint8>(x, y, 0, static_cast<uint8>((x * 5 + y * 3) & 255));
                big.set<uint8>(x, y, 1, static_cast<uint8>((x * 9 + y * 7) & 255));
                big.set<uint8>(x, y, 2, static_cast<uint8>((x * 11 + y * 13) & 255));
            }
        QuantizeParams pp;
        pp.bitsR = 5; pp.bitsG = 6; pp.bitsB = 5; pp.dither = DitherMode::Ordered4x4;
        Image qs(ImageFormat(PixelFormat::RGB, DataType::UInt8), 200, 120, ColorSpace::SRGB);
        Image qp(ImageFormat(PixelFormat::RGB, DataType::UInt8), 200, 120, ColorSpace::SRGB);
        quantizeImage(big.view(), qs.view(), pp);
        quantizeImage(big.view(), qp.view(), pp, ExecutionPolicy::simdParallel());
        bool qeq = true;
        for (uint32 y = 0; y < 120 && qeq; ++y)
            for (uint32 x = 0; x < 200 && qeq; ++x)
                for (uint32 c = 0; c < 3; ++c)
                    if (qs.get<uint8>(x, y, c) != qp.get<uint8>(x, y, c)) { qeq = false; break; }
        TEST_CHECK(qeq);
    }

    // --- Palette + PackedImage ---
    {
        Image src(ImageFormat(PixelFormat::RGB, DataType::UInt8), 48, 48, ColorSpace::SRGB);
        for (uint32 y = 0; y < 48; ++y)
            for (uint32 x = 0; x < 48; ++x) {
                uint8_t v = static_cast<uint8_t>((x * 16 + y)) % 256;
                src.set<uint8_t>(x, y, 0, v);
                src.set<uint8_t>(x, y, 1, static_cast<uint8_t>(255 - v));
                src.set<uint8_t>(x, y, 2, static_cast<uint8_t>(64));
            }
        byte pal[16 * 4];
        size_t pc = buildPalette(src.view(), 16, pal);
        TEST_CHECK(pc >= 1 && pc <= 16);

        PackedImage packImg(src, PackedFormat::Indexed8, pal, pc);
        TEST_CHECK(packImg.pixelCount() == 48 * 48);
        TEST_CHECK(packImg.format() == PackedFormat::Indexed8);
        TEST_CHECK(packImg.rowBytes() >= 48);
        Image back = packImg.unpack();
        TEST_CHECK(back.width() == 48 && back.height() == 48 && back.channels() == 4);

        PackedImage r565(src, PackedFormat::RGB565);
        TEST_CHECK(r565.bitDepth() == 16);
        Image back2 = r565.unpack();
        TEST_CHECK(back2.channels() == 3);
        TEST_CHECK(nearV(back2.get<uint8_t>(1, 1, 0), src.get<uint8_t>(1, 1, 0), 8) <= 8);

        PackedImage gray1(src, PackedFormat::Gray1);
        TEST_CHECK(gray1.bitDepth() == 1);
        TEST_CHECK(gray1.rowBytes() == 8); // 48px @1bpp = 6B aligned to 4 -> 8
        Image g1 = gray1.unpack();
        TEST_CHECK(g1.channels() == 1);

        PackedImage g16 = packImg.clone();
        TEST_CHECK(g16.pixelCount() == packImg.pixelCount());
        TEST_CHECK(g16.readPixel(3, 3) == packImg.readPixel(3, 3));
        TEST_CHECK(g16.paletteCount() == pc);
    }

    return true;
}