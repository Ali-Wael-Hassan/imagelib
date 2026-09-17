#pragma once
#ifndef IMAGELIB_CORE_DATATYPES_H_
#define IMAGELIB_CORE_DATATYPES_H_
/// @file
/// Image scalar data type and its properties.

#include "imagelib/core/types/FixedTypes.h"

#include <cstddef>

namespace iml {

/// Image scalar data type.
enum class DataType : uint8 {
    /// No image / unset marker.
    Unknown = 0,
    /// 8-bit unsigned integer.
    UInt8,
    /// 16-bit unsigned integer.
    UInt16,
    /// 32-bit unsigned integer.
    UInt32,
    /// 16-bit floating point.
    Float16,
    /// 32-bit floating point.
    Float32,
};

/// Returns a printable name for a data type.
/// @param t The data type to convert.
/// @return A NUL-terminated string naming `t`, or "Unknown".
constexpr const char* toString(DataType t) noexcept {
    switch (t) {
    case DataType::Unknown:
        return "Unknown";
    case DataType::UInt8:
        return "UInt8";
    case DataType::UInt16:
        return "UInt16";
    case DataType::UInt32:
        return "UInt32";
    case DataType::Float16:
        return "Float16";
    case DataType::Float32:
        return "Float32";
    }
    return "Unknown";
}

/// Size in bytes of one scalar sample (+1) for a data type. 0 when unsupported
/// as a storage type.
/// @param t The data type to query.
/// @return The byte size of one sample, or 0 for unsupported types.
constexpr size_t dataTypeSize(DataType t) noexcept {
    switch (t) {
    case DataType::UInt8:
        return sizeof(uint8);
    case DataType::UInt16:
        return sizeof(uint16);
    case DataType::UInt32:
        return sizeof(uint32);
    case DataType::Float32:
        return sizeof(float);
    case DataType::Float16:
        return sizeof(uint16);
    case DataType::Unknown:
        return 0;
    }
    return 0;
}

} // namespace iml

#endif