// imagelib/src/math/Scalar.cpp
//
// Out-of-line bridged scalar wrappers for Scalar.h (float/double overloads of
// the standard library math functions).

#include "imagelib/math/Scalar.h"

#include <cmath>

namespace iml {
namespace math {

float floor_(float x)  noexcept { return std::floor(x); }
double floor_(double x) noexcept { return std::floor(x); }

float ceil_(float x)   noexcept { return std::ceil(x); }
double ceil_(double x) noexcept { return std::ceil(x); }

float round_(float x)  noexcept { return std::floor(x + 0.5f); }
double round_(double x) noexcept { return std::floor(x + 0.5); }

float sqrt_(float x)  noexcept { return std::sqrt(x); }
double sqrt_(double x) noexcept { return std::sqrt(x); }

float pow_(float base, float exp_) noexcept { return std::pow(base, exp_); }
double pow_(double base, double exp_) noexcept { return std::pow(base, exp_); }

float exp_(float x) noexcept { return std::exp(x); }
double exp_(double x) noexcept { return std::exp(x); }

float log_(float x) noexcept { return std::log(x); }
double log_(double x) noexcept { return std::log(x); }

float sin_(float x) noexcept { return std::sin(x); }
double sin_(double x) noexcept { return std::sin(x); }

float cos_(float x) noexcept { return std::cos(x); }
double cos_(double x) noexcept { return std::cos(x); }

float tan_(float x) noexcept { return std::tan(x); }
double tan_(double x) noexcept { return std::tan(x); }

float asin_(float x) noexcept { return std::asin(x); }
double asin_(double x) noexcept { return std::asin(x); }

float acos_(float x) noexcept { return std::acos(x); }
double acos_(double x) noexcept { return std::acos(x); }

float atan_(float x) noexcept { return std::atan(x); }
double atan_(double x) noexcept { return std::atan(x); }

float atan2_(float y, float x) noexcept { return std::atan2(y, x); }
double atan2_(double y, double x) noexcept { return std::atan2(y, x); }

} // namespace math
} // namespace iml