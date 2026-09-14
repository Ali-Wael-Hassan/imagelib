// examples/basic_demo.cpp
//
// Basic ImageLib usage: load an image, convert to grayscale via the library's
// built-in toGray(), and save the result as PNG and BMP.
//
// Build:
//   mingw32-make -C build
// Run:
//   bin/basic_demo.exe assets/mario.bmp output.png

#include <iostream>
#include <string>

#include "imagelib/imagelib.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: basic_demo <input> <output.png|output.bmp>\n";
        std::cout << "  Converts the input image to grayscale and saves it.\n";
        return 1;
    }

    const std::string inPath  = argv[1];
    const std::string outPath = argv[2];

    iml::Image src(inPath);
    std::cout << "Loaded: " << src.width() << "x" << src.height()
              << " (" << static_cast<int>(src.channels()) << "ch)\n";

    iml::Image gray(iml::fmt::gray8, src.width(), src.height(), iml::ColorSpace::Gray);
    iml::proc::color::toGray(src.view(), gray.view());

    gray.save(outPath);
    std::cout << "Saved grayscale: " << outPath << "\n";
    return 0;
}