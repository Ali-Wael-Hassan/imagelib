#ifndef IMAGELIB_CORE_IMAGE_H_
#error "Include Image.h, not Image.tpp directly."
#endif

#include "imagelib/core/Types.h"
#include "imagelib/core/image/ImageView.h"

namespace iml {

/// Reads sample channel c of pixel (x, y) converted to T.
/// @tparam T The sample type to read.
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @return The sample as T.
/// @throws InvalidParameterError when out of bounds.
/// @throws UnsupportedFormatError on unsupported data types.
template <typename T> T Image::get(int32 x, int32 y, uint32 c) const {
    return view().template getSample<T>(x, y, c);
}

/// Writes a sample whose store width matches the image data type.
/// @tparam T The sample type to write.
/// @param x Column index.
/// @param y Row index.
/// @param c Channel index.
/// @param value The sample value.
/// @throws InvalidParameterError when out of bounds.
/// @throws UnsupportedFormatError on a size mismatch.
template <typename T> void Image::set(int32 x, int32 y, uint32 c, T value) {
    view().template setSample<T>(x, y, c, value);
}

} // namespace iml