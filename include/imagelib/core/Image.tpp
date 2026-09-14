// imagelib/core/Image.tpp
//
// Template member definitions for Image.h. This file is included at the
// bottom of Image.h (never include it directly).

#ifndef IMAGELIB_CORE_IMAGE_H_
#error "Include Image.h, not Image.tpp directly."
#endif

#include "imagelib/core/Types.h"
#include "imagelib/core/ImageView.h"

namespace iml {

template <typename T>
T Image::get(int32 x, int32 y, uint32 c) const {
    return view().template getSample<T>(x, y, c);
}

template <typename T>
void Image::set(int32 x, int32 y, uint32 c, T value) {
    view().template setSample<T>(x, y, c, value);
}

} // namespace iml