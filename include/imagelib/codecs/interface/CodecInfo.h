#ifndef IMAGELIB_CODECS_CODECINFO_H_
#define IMAGELIB_CODECS_CODECINFO_H_
#pragma once
/// @file CodecInfo.h
/// Static capabilities of an image codec.

#include "imagelib/codecs/interface/Format.h"
#include "imagelib/core/Types.h"

#include <string>
#include <vector>

namespace iml {
namespace codecs {

/// Static capabilities of an image codec.
struct CodecInfo {
    /// File format handled by this codec.
    ImageCodecFormat format = ImageCodecFormat::Unknown;

    /// Canonical lowercase extension WITHOUT dot (e.g. "png").
    std::string extension;

    /// Formats this codec can decode into (inputs).
    std::vector<PixelFormat> readablePixels;

    /// Formats this codec can encode from (outputs).
    std::vector<PixelFormat> writablePixels;

    /// True when this codec can decode pixels in \p pf.
    /// @param pf Pixel format to query.
    /// @return True when decodable.
    bool canRead(PixelFormat pf) const noexcept {
        for (const auto& p : readablePixels)
            if (p == pf)
                return true;
        return false;
    }
    /// True when this codec can encode pixels in \p pf.
    /// @param pf Pixel format to query.
    /// @return True when encodable.
    bool canWrite(PixelFormat pf) const noexcept {
        for (const auto& p : writablePixels)
            if (p == pf)
                return true;
        return false;
    }
};

} // namespace codecs
} // namespace iml

#endif