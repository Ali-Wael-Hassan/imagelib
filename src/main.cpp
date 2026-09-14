// imagelib demo: converts a loaded image to grayscale and saves it.

#include <iostream>
#include <string>

#include "imagelib/imagelib.h"

int main() {
    std::string filename;
    std::cout << "Pls enter colored image name to turn to gray scale: ";
    std::cin >> filename;

    iml::Image image(filename);

    for (uint32_t i = 0; i < image.width(); ++i) {
        for (uint32_t j = 0; j < image.height(); ++j) {
            unsigned int avg = 0;
            for (uint32_t k = 0; k < 3; ++k) {
                avg += image(i, j, k);
            }
            avg /= 3;
            image(i, j, 0) = static_cast<unsigned char>(avg);
            image(i, j, 1) = static_cast<unsigned char>(avg);
            image(i, j, 2) = static_cast<unsigned char>(avg);
        }
    }

    std::cout << "Pls enter image name to store new image\n";
    std::cout << "and specify extension .jpg, .bmp, .png, .tga: ";

    std::cin >> filename;
    image.save(filename);

    return 0;
}