#pragma once
#define IMAGELIB_PROCESSING_RESIZE_H_
// imagelib/processing/Resize.h
//
// Geometric resampling: nearest-neighbor and bilinear. Nearest picks the pixel
// under the scaled coordinate; bilinear uses center-based coordinate mapping
// with border handling for out-of-bounds taps.

#include "imagelib/processing/Pixel.h"
#include "imagelib/processing/Convolution.h"

#include <cmath>

namespace iml {
namespace proc {
namespace resize {

enum class Filter : uint8 { Nearest = 0, Bilinear = 1 };

void resize(const ConstImageView& src, ImageView dst, Filter f,
            conv::BorderMode border = conv::BorderMode::Clamp);

/// Row-parallel resize (nearest or bilinear). Falls back to the scalar path
/// for tiny images.
void resize(const ConstImageView& src, ImageView dst, Filter f,
            conv::BorderMode border, const ExecutionPolicy& policy);

} // namespace resize
} // namespace proc
} // namespace iml