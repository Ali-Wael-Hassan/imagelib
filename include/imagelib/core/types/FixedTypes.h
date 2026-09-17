#pragma once
#ifndef IMAGELIB_CORE_FIXEDTYPES_H_
#define IMAGELIB_CORE_FIXEDTYPES_H_
/// @file
/// Fixed-width integral aliases used across ImageLib.

#include <cstdint>

namespace iml {

/// 8-bit signed integer.
using int8 = std::int8_t;
/// 8-bit unsigned integer.
using uint8 = std::uint8_t;
/// 16-bit signed integer.
using int16 = std::int16_t;
/// 16-bit unsigned integer.
using uint16 = std::uint16_t;
/// 32-bit signed integer.
using int32 = std::int32_t;
/// 32-bit unsigned integer.
using uint32 = std::uint32_t;
/// 64-bit signed integer.
using int64 = std::int64_t;
/// 64-bit unsigned integer.
using uint64 = std::uint64_t;

/// Unsigned byte alias for uint8.
using byte = uint8;

} // namespace iml

#endif