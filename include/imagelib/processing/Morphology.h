#pragma once
#define IMAGELIB_PROCESSING_MORPHOLOGY_H_
// imagelib/processing/Morphology.h
//
// Grayscale/binary morphology. Structuring elements are small offset sets.
// erode = minimum, dilate = maximum over the element; open/close compose them.
// Operates per channel on normalized samples, so it works for any storage type.

#include "imagelib/processing/Pixel.h"
#include "imagelib/processing/Convolution.h"
#include "imagelib/processing/Transform.h"
#include "imagelib/core/Image.h"

#include <algorithm>
#include <vector>

namespace iml {
namespace proc {
namespace morph {

/// Structuring element: a set of (dx, dy) offsets relative to its origin.
struct StructuringElement {
    std::vector<std::pair<int32, int32>> offsets;

    bool empty() const noexcept;
    size_t size() const noexcept;
};

/// Square of radius r (side 2r+1). r = 0 yields a single-pixel element.
StructuringElement square(int radius);

/// Cross of arm length r (center + N/S/E/W points).
StructuringElement cross(int radius);

void checkElement(const StructuringElement& e);

void erode(const ConstImageView& src, ImageView dst, const StructuringElement& e,
           conv::BorderMode border = conv::BorderMode::Clamp);

void dilate(const ConstImageView& src, ImageView dst, const StructuringElement& e,
            conv::BorderMode border = conv::BorderMode::Clamp);

/// Morphological opening: erode then dilate with the same element.
void open(const ConstImageView& src, ImageView dst, const StructuringElement& e,
          conv::BorderMode border = conv::BorderMode::Clamp);

/// Morphological closing: dilate then erode with the same element.
void close(const ConstImageView& src, ImageView dst, const StructuringElement& e,
           conv::BorderMode border = conv::BorderMode::Clamp);

} // namespace morph
} // namespace proc
} // namespace iml