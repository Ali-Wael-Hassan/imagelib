#ifndef IMAGELIB_CODECS_FORMAT_H_
#define IMAGELIB_CODECS_FORMAT_H_
#pragma once
/// @file Format.h
/// Image codec file format enumeration.

#include "imagelib/core/Types.h"

namespace iml {
namespace codecs {

/// Supported image codec file formats.
enum class ImageCodecFormat : uint16 {
    /// Format not recognized.
    Unknown = 0,
    /// Portable Network Graphics.
    PNG,
    /// Joint Photographic Experts Group.
    JPEG,
    /// Windows Bitmap.
    BMP,
    /// Truevision Targa.
    TGA,
};

} // namespace codecs
} // namespace iml

#endif