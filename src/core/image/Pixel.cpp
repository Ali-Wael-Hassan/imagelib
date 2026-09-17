#include "imagelib/core/image/Pixel.h"
#include "imagelib/math/Scalar.h"

#include <cstdint>

namespace iml {

/// Adds another pixel component-wise in place.
/// @param o The other pixel.
/// @return Reference to this pixel.
Pixel& Pixel::operator+=(const Pixel& o) noexcept {
    r += o.r;
    g += o.g;
    b += o.b;
    a += o.a;
    return *this;
}
/// Subtracts another pixel component-wise in place.
/// @param o The other pixel.
/// @return Reference to this pixel.
Pixel& Pixel::operator-=(const Pixel& o) noexcept {
    r -= o.r;
    g -= o.g;
    b -= o.b;
    a -= o.a;
    return *this;
}
/// Multiplies another pixel component-wise in place.
/// @param o The other pixel.
/// @return Reference to this pixel.
Pixel& Pixel::operator*=(const Pixel& o) noexcept {
    r *= o.r;
    g *= o.g;
    b *= o.b;
    a *= o.a;
    return *this;
}
/// Scales all channels in place.
/// @param s The scalar multiplier.
/// @return Reference to this pixel.
Pixel& Pixel::operator*=(float s) noexcept {
    r *= s;
    g *= s;
    b *= s;
    a *= s;
    return *this;
}
/// Divides all channels in place.
/// @param s The scalar divisor.
/// @return Reference to this pixel.
Pixel& Pixel::operator/=(float s) noexcept {
    r /= s;
    g /= s;
    b /= s;
    a /= s;
    return *this;
}

/// Returns the weighted luminance of the RGB channels (alpha ignored).
/// @return Luminance in [0, 1].
float Pixel::luminance() const noexcept { return 0.2126f * r + 0.7152f * g + 0.0722f * b; }

} // namespace iml