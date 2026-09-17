#include "imagelib/processing/Convolution.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace iml {
namespace proc {
namespace conv {

/// Construct a kernel from dimensions and row-major weight data.
/// @param w Kernel width.
/// @param h Kernel height.
/// @param d Row-major weight data (width*height entries).
Kernel::Kernel(int w, int h, std::vector<float> d) : width(w), height(h), data(std::move(d)) {}

/// True when dimensions are positive odd numbers and data matches.
/// @return Validity of the kernel state.
bool Kernel::valid() const noexcept {
    return width > 0 && height > 0 && data.size() == static_cast<size_t>(width) * height &&
           (width & 1) == 1 && (height & 1) == 1;
}

/// Weight at local offset (kx, ky); (0,0) is the kernel center.
/// @param kx,ky Local offsets from the kernel center.
/// @return Weight value.
float Kernel::at(int kx, int ky) const noexcept {
    return data[static_cast<size_t>(ky) * width + kx];
}

/// X radius (width / 2).
/// @return X radius.
int Kernel::radiusX() const noexcept { return width / 2; }
/// Y radius (height / 2).
/// @return Y radius.
int Kernel::radiusY() const noexcept { return height / 2; }

/// Apply the kernel at (x, y, c) using normalized [0,1] samples.
/// @param img Source image.
/// @param x,y,c Sample coordinate and channel.
/// @param border Out-of-bounds border mode.
/// @return Weighted sum of normalized samples.
float Kernel::applyNorm(const ConstImageView& img, int32 x, int32 y, uint32 c, BorderMode border)
    const noexcept {
    const int rx = radiusX();
    const int ry = radiusY();
    float acc = 0.f;
    for (int ky = -ry; ky <= ry; ++ky)
        for (int kx = -rx; kx <= rx; ++kx) {
            const float wgt = at(kx + rx, ky + ry);
            if (wgt != 0.f)
                acc += wgt * detail::sampleNorm(img, x + kx, y + ky, c, border);
        }
    return acc;
}

/// Apply the kernel at (x, y) reading canonical Pixels.
/// @param img Source image.
/// @param x,y Sample coordinate.
/// @param border Out-of-bounds border mode.
/// @return Weighted-sum canonical Pixel.
Pixel Kernel::applyPixel(const ConstImageView& img, int32 x, int32 y, BorderMode border)
    const noexcept {
    const uint16 cc = img.channels();
    const float r = cc > 0 ? applyNorm(img, x, y, 0, border) : 0.f;
    const float g = cc > 1 ? applyNorm(img, x, y, 1, border) : r;
    const float b = cc > 2 ? applyNorm(img, x, y, 2, border) : r;
    const float a = cc > 3 ? applyNorm(img, x, y, 3, border) : 1.f;
    return Pixel(r, g, b, a);
}

} // namespace conv
} // namespace proc
} // namespace iml