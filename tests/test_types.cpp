// tests/test_types.cpp – Core Types API
#include "tests.h"
#include "imagelib/core/Types.h"
#include <cstring>

using namespace iml;

bool test_types() {
    // DataType sizes
    TEST_CHECK(dataTypeSize(DataType::UInt8)   == 1);
    TEST_CHECK(dataTypeSize(DataType::UInt16)  == 2);
    TEST_CHECK(dataTypeSize(DataType::UInt32)  == 4);
    TEST_CHECK(dataTypeSize(DataType::Float16) == 2);
    TEST_CHECK(dataTypeSize(DataType::Float32) == 4);
    TEST_CHECK(dataTypeSize(DataType::Unknown) == 0);

    TEST_CHECK(strcmp(toString(DataType::UInt8), "UInt8") == 0);
    TEST_CHECK(strcmp(toString(DataType::Unknown), "Unknown") == 0);

    // PixelFormat channels
    TEST_CHECK(pixelFormatChannels(PixelFormat::Gray)       == 1);
    TEST_CHECK(pixelFormatChannels(PixelFormat::GrayAlpha)  == 2);
    TEST_CHECK(pixelFormatChannels(PixelFormat::RGB)        == 3);
    TEST_CHECK(pixelFormatChannels(PixelFormat::BGR)        == 3);
    TEST_CHECK(pixelFormatChannels(PixelFormat::RGBA)       == 4);
    TEST_CHECK(pixelFormatChannels(PixelFormat::BGRA)       == 4);
    TEST_CHECK(pixelFormatChannels(PixelFormat::Unknown)    == 0);
    TEST_CHECK(pixelFormatChannels(PixelFormat::Packed)     == 0);

    // ImageFormat
    ImageFormat f(PixelFormat::RGB, DataType::UInt8);
    TEST_CHECK(f.valid());
    TEST_CHECK(f.channels() == 3);
    TEST_CHECK(f.sampleSize() == 1);
    TEST_CHECK(f.pixelSize() == 3);
    TEST_CHECK(f == fmt::rgb8);
    TEST_CHECK(f != fmt::gray8);

    ImageFormat def;
    TEST_CHECK(!def.valid());

    // Presets
    TEST_CHECK(fmt::gray8   == ImageFormat(PixelFormat::Gray,    DataType::UInt8));
    TEST_CHECK(fmt::gray16  == ImageFormat(PixelFormat::Gray,    DataType::UInt16));
    TEST_CHECK(fmt::gray32f == ImageFormat(PixelFormat::Gray,    DataType::Float32));
    TEST_CHECK(fmt::rgb8    == ImageFormat(PixelFormat::RGB,     DataType::UInt8));
    TEST_CHECK(fmt::rgba8   == ImageFormat(PixelFormat::RGBA,    DataType::UInt8));
    TEST_CHECK(fmt::bgr8    == ImageFormat(PixelFormat::BGR,     DataType::UInt8));

    // ColorSpace
    TEST_CHECK(strcmp(toString(ColorSpace::SRGB), "sRGB") == 0);
    TEST_CHECK(strcmp(toString(ColorSpace::Linear), "Linear") == 0);
    TEST_CHECK(strcmp(toString(ColorSpace::Unknown), "Unknown") == 0);

    return true;
}