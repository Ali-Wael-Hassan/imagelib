// tests/test_image.cpp – Image container
#include "tests.h"
#include "imagelib/core/Image.h"
#include <cstring>

using namespace iml;

bool test_image() {
    {
        Image img(fmt::rgb8, 16, 8, ColorSpace::SRGB);
        TEST_CHECK(img.width() == 16 && img.height() == 8);
        TEST_CHECK(img.channels() == 3);
        TEST_CHECK(img.pixelFormat() == PixelFormat::RGB);
        TEST_CHECK(img.dataType() == DataType::UInt8);
        TEST_CHECK(img.colorSpace() == ColorSpace::SRGB);
        TEST_CHECK(!img.empty());
        TEST_CHECK(img.pixelSizeBytes() == 3);
        TEST_CHECK(img.bytesPerChannel() == 1);
        TEST_CHECK(img.size().width == 16 && img.size().height == 8);
    }
    {
        // set/get typed roundtrip (RGBA)
        Image img(fmt::rgba8, 4, 4, ColorSpace::SRGB, AlphaMode::Straight);
        img.set<uint8_t>(1, 2, 0, 200);
        img.set<uint8_t>(1, 2, 1, 100);
        img.set<uint8_t>(1, 2, 2, 50);
        img.set<uint8_t>(1, 2, 3, 255);
        TEST_CHECK(img.get<uint8_t>(1, 2, 0) == 200);
        TEST_CHECK(img.get<uint8_t>(1, 2, 1) == 100);
        TEST_CHECK(img.get<uint8_t>(1, 2, 2) == 50);
        TEST_CHECK(img.get<uint8_t>(1, 2, 3) == 255);
        TEST_CHECK(img.hasAlpha());
    }
    {
        // Float32 image
        Image img(fmt::gray32f, 8, 8, ColorSpace::Linear);
        img.set<float>(3, 4, 0, 0.5f);
        TEST_CHECK_NEAR(img.get<float>(3, 4, 0), 0.5f, 1e-6f);
        TEST_CHECK(img.pixelSizeBytes() == 4);
    }
    {
        // stride respects row alignment
        Image img(fmt::rgb8, 3, 3, ColorSpace::SRGB, AlphaMode::None, 16);
        TEST_CHECK(img.stride() >= 9);
        TEST_CHECK(img.stride() % 16 == 0);
        TEST_CHECK_NEAR(static_cast<uint64>(img.stride()) * img.height(), img.bufferBytes(), 0);
    }
    {
        // clone deep-copies
        Image img(fmt::rgb8, 8, 4);
        img.set<uint8_t>(5, 2, 0, 11);
        Image c = img.clone();
        TEST_CHECK(c.width() == 8 && c.height() == 4);
        TEST_CHECK(c.get<uint8_t>(5, 2, 0) == 11);
        c.set<uint8_t>(5, 2, 0, 99);
        TEST_CHECK(img.get<uint8_t>(5, 2, 0) == 11);
        TEST_CHECK(c.get<uint8_t>(5, 2, 0) == 99);
    }
    {
        // assignment copies; move empties source
        Image a(fmt::gray8, 10, 10);
        a.set<uint8_t>(0, 0, 0, 7);
        Image b = a;
        TEST_CHECK(b.width() == 10 && b.get<uint8_t>(0, 0, 0) == 7);
        Image m = std::move(b);
        TEST_CHECK(m.width() == 10 && m.get<uint8_t>(0, 0, 0) == 7);
        TEST_CHECK(b.empty());
    }
    {
        // release empties
        Image img(fmt::gray16, 4, 4);
        TEST_CHECK(!img.empty());
        img.release();
        TEST_CHECK(img.empty());
        TEST_CHECK(img.width() == 0);
    }
    {
        // raw access via data()/row()/pixel()/sample()
        Image img(fmt::gray8, 2, 2);
        img.set<uint8_t>(1, 1, 0, 42);
        TEST_CHECK(img.data() != nullptr);
        TEST_CHECK(img.sample(1, 1, 0)[0] == 42);
        byte*  p = img.pixel(1, 1);
        TEST_CHECK(p[0] == 42);
        byte*  r = img.row(1);
        TEST_CHECK(r[1] == 42);
        TEST_CHECK(&img(1, 1, 0) == p);
        TEST_CHECK(img(1, 1, 0) == 42);
    }
    {
        // default-constructed image is empty
        Image def;
        TEST_CHECK(def.empty());
        TEST_CHECK(def.format() == ImageFormat());
    }
    return true;
}