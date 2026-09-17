#ifndef IMAGELIB_SRC_CORE_IMAGEUTILS_H_
#define IMAGELIB_SRC_CORE_IMAGEUTILS_H_

/// @file ImageUtils.h
/// Internal helpers used by the Image implementation; not a public header.

#include "imagelib/core/Types.h"
#include "imagelib/core/memory/Memory.h"

namespace iml {

/// Compute the padded row stride for an image with the given parameters.
/// @param width Image width in pixels.
/// @param channels Channels per pixel.
/// @param bpc Bytes per channel.
/// @param rowAlign Row alignment in bytes.
/// @return The padded row stride in bytes.
inline uint32 computeStride(uint32 width, uint16 channels, uint32 bpc, uint32 rowAlign) {
    uint64 rowBytes = static_cast<uint64>(width) * channels * bpc;
    if (rowAlign > 1) {
        rowBytes = mem::alignUp(rowBytes, rowAlign);
    }
    return static_cast<uint32>(rowBytes);
}

} // namespace iml

#endif
