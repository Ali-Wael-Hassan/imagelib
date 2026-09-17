// ImageLib Tutorial 01 - Images
// ---------------------------------
// The production things you will actually use every day:
//   * Image  - owns the pixels (load, save, metadata, pixel access)
//   * ImageView / ConstImageView - cheap "windows" over an Image
//   * Pixel  - one RGBA pixel as floats in [0, 1]
//   * pixel::readPixel / writeRGBA / writeNorm - the filter-writing toolkit
//
// Notice how grayscale and invert are each ~5 lines of your own math. The
// library is the *tool*: it loads/owns/saves the pixels and moves them in and
// out as float RGBA. The filter itself is yours to write - that is exactly the
// shape the assignment filters take.
//
// Build + run (from the repository root):
//   g++ -std=c++17 -O2 -Iinclude -Isrc tutorial/01_image.cpp -Lbin \
//       -limagelib -lstb_image -o bin/tutorial_01_image.exe
//   ./bin/tutorial_01_image.exe [input_image] [output_folder]
//
// Example:
//   ./bin/tutorial_01_image.exe assets/mario.bmp

#include "imagelib/imagelib.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace iml;

namespace {

// Try the user-supplied path first, then a few well-known fallbacks so the
// demo works no matter which folder it is launched from.
// Returns true when the file exists on disk (used only to pick a demo image).
bool fileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

std::string pickInput(const std::string& given) {
    if (!given.empty())
        return given;
    const char* candidates[] = {
        "assets/mario.bmp",
        "../assets/mario.bmp",
        "../../assets/mario.bmp",
    };
    for (const char* c : candidates)
        if (fileExists(c))
            return c;
    return "assets/mario.bmp";
}

std::string outputPath(const std::string& folder, const std::string& file) {
    if (folder.empty())
        return file;
    return folder + "/" + file;
}

// Grayscale, written BY HAND. readPixel hands us one pixel as float RGBA, we
// compute the Rec.709 luma, writeNorm puts a single channel back into the
// gray8 destination. That is the whole filter - a small loop, not a distant
// library call.
Image grayManual(const ConstImageView& src) {
    Image out(fmt::gray8, src.width(), src.height(), ColorSpace::Gray);
    ImageView dv = out.view();
    for (int32 y = 0; y < (int32)src.height(); ++y)
        for (int32 x = 0; x < (int32)src.width(); ++x) {
            const Pixel p = pixel::readPixel(src, x, y);
            const float l = math::clamp01(0.2126f * p.r + 0.7152f * p.g + 0.0722f * p.b);
            pixel::writeNorm(dv, x, y, 0, l);
        }
    return out;
}

// Invert, written BY HAND the same way. Per channel: 1 minus the value.
// Having written these two, you know the exact shape the assignment filters
// take: load -> loop pixels -> write. Later lessons add the parallel/SIMD
// loops that make the same math fast.
Image invertManual(const ConstImageView& src) {
    Image out(src.format(), src.width(), src.height(), src.colorSpace(), src.alphaMode());
    ImageView dv = out.view();
    for (int32 y = 0; y < (int32)src.height(); ++y)
        for (int32 x = 0; x < (int32)src.width(); ++x) {
            const Pixel p = pixel::readPixel(src, x, y);
            pixel::writeRGBA(Pixel(1.f - p.r, 1.f - p.g, 1.f - p.b, p.a), dv, x, y);
        }
    return out;
}

} // namespace

int main(int argc, char** argv) {
    const std::string input = pickInput(argc > 1 ? argv[1] : "");
    const std::string outDir = argc > 2 ? argv[2] : "";

    try {
        // ---- 1. Load --------------------------------------------------------
        Image img(input);
        std::cout << "Loaded  : " << input << '\n';
        std::cout << "Size    : " << img.width() << " x " << img.height() << '\n';
        std::cout << "Channels: " << img.channels() << '\n';
        std::cout << "Data    : " << toString(img.dataType())
                  << "  color space: " << toString(img.colorSpace())
                  << "  alpha: " << (img.hasAlpha() ? "yes" : "no") << '\n';

        // ---- 2. Save a copy ------------------------------------------------
        img.save(outputPath(outDir, "01_copy.png"));
        std::cout << "Saved   : " << outputPath(outDir, "01_copy.png") << '\n';

        // ---- 3. Grayscale, hand-written ------------------------------------
        Image gray = grayManual(img.view());
        gray.save(outputPath(outDir, "01_gray.png"));
        std::cout << "Saved   : " << outputPath(outDir, "01_gray.png") << '\n';

        // ---- 4. Invert, hand-written ---------------------------------------
        Image inv = invertManual(img.view());
        inv.save(outputPath(outDir, "01_invert.png"));
        std::cout << "Saved   : " << outputPath(outDir, "01_invert.png") << '\n';

        // ---- 5. One pixel, in and out --------------------------------------
        const Pixel center = img.readPixel((int32)(img.width() / 2), (int32)(img.height() / 2));
        std::cout << "Center pixel RGBA = "
                  << center.r << ", " << center.g << ", " << center.b << ", " << center.a << '\n';

        std::cout << "\nAll good. Open the saved files to see the results.\n";
        std::cout << "Your take-away: the library loads, stores and moves pixels;\n";
        std::cout << "the filter loop is yours to write (that is the assignment).\n";
        return 0;
    } catch (const Error& e) {
        std::cerr << "ImageLib error: " << e.what() << '\n';
        return 1;
    }
}