#include "imagelib/math/Scalar.h"

#include <cmath>

namespace iml {
namespace math {

/// Returns the largest integral value not greater than x.
/// @param x Input value.
/// @return The floored value.
float floor_(float x) noexcept { return std::floor(x); }
double floor_(double x) noexcept { return std::floor(x); }

/// Returns the smallest integral value not less than x.
/// @param x Input value.
/// @return The ceiled value.
float ceil_(float x) noexcept { return std::ceil(x); }
double ceil_(double x) noexcept { return std::ceil(x); }

/// Rounds x to the nearest integral value.
/// @param x Input value.
/// @return The rounded value.
float round_(float x) noexcept { return std::floor(x + 0.5f); }
double round_(double x) noexcept { return std::floor(x + 0.5); }

/// Returns the square root of x.
/// @param x Input value.
/// @return The square root of x.
float sqrt_(float x) noexcept { return std::sqrt(x); }
double sqrt_(double x) noexcept { return std::sqrt(x); }

/// Returns base raised to the power exp_.
/// @param base Base value.
/// @param exp_ Exponent.
/// @return base ^ exp_.
float pow_(float base, float exp_) noexcept { return std::pow(base, exp_); }
double pow_(double base, double exp_) noexcept { return std::pow(base, exp_); }

/// Returns e raised to the power x.
/// @param x Input value.
/// @return The exponential of x.
float exp_(float x) noexcept { return std::exp(x); }
double exp_(double x) noexcept { return std::exp(x); }

/// Returns the natural logarithm of x.
/// @param x Input value.
/// @return The natural logarithm of x.
float log_(float x) noexcept { return std::log(x); }
double log_(double x) noexcept { return std::log(x); }

/// Returns the sine of x in radians.
/// @param x Angle in radians.
/// @return The sine of x.
float sin_(float x) noexcept { return std::sin(x); }
double sin_(double x) noexcept { return std::sin(x); }

/// Returns the cosine of x in radians.
/// @param x Angle in radians.
/// @return The cosine of x.
float cos_(float x) noexcept { return std::cos(x); }
double cos_(double x) noexcept { return std::cos(x); }

/// Returns the tangent of x in radians.
/// @param x Angle in radians.
/// @return The tangent of x.
float tan_(float x) noexcept { return std::tan(x); }
double tan_(double x) noexcept { return std::tan(x); }

/// Returns the arcsine of x in [-pi/2, pi/2].
/// @param x Input in [-1, 1].
/// @return The arcsine of x in radians.
float asin_(float x) noexcept { return std::asin(x); }
double asin_(double x) noexcept { return std::asin(x); }

/// Returns the arccosine of x in [0, pi].
/// @param x Input in [-1, 1].
/// @return The arccosine of x in radians.
float acos_(float x) noexcept { return std::acos(x); }
double acos_(double x) noexcept { return std::acos(x); }

/// Returns the arctangent of x in [-pi/2, pi/2].
/// @param x Input value.
/// @return The arctangent of x in radians.
float atan_(float x) noexcept { return std::atan(x); }
double atan_(double x) noexcept { return std::atan(x); }

/// Returns the four-quadrant arctangent of y/x in [-pi, pi].
/// @param y Y coordinate.
/// @param x X coordinate.
/// @return The angle in radians.
float atan2_(float y, float x) noexcept { return std::atan2(y, x); }
double atan2_(double y, double x) noexcept { return std::atan2(y, x); }

} // namespace math
} // namespace iml