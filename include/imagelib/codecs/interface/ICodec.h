#ifndef IMAGELIB_CODECS_ICODEC_H_
#define IMAGELIB_CODECS_ICODEC_H_
#pragma once
/// @file ICodec.h
/// Abstract codec interface: one object per file format.

#include "imagelib/codecs/interface/CodecInfo.h"
#include "imagelib/core/image/Image.h"

namespace iml {
namespace codecs {

/// Abstract codec: one object per file format.
class ICodec {
  public:
    virtual ~ICodec() = default;
    /// Returns the static capabilities of this codec.
    virtual const CodecInfo& info() const = 0;

    /// Magic-byte sniffing (returns False when not feasible, e.g. TGA).
    /// @param data Pointer to the file's leading bytes.
    /// @param size Number of available bytes.
    /// @return True when the bytes match this codec's signature.
    virtual bool canReadHeader(const byte* data, size_t size) const noexcept { return false; }

    /// Decodes `path` into `out` (out is fully replaced).
    /// @param out Image that receives the decoded pixels.
    /// @param path Path of the file to decode.
    virtual void read(Image& out, const std::string& path) = 0;

    /// Encodes the image into `path`.
    /// @param src Image to encode.
    /// @param path Destination file path.
    /// @param opts Save options to pass to the encoder.
    virtual void write(const Image& src, const std::string& path, const SaveOptions& opts) = 0;
};

} // namespace codecs
} // namespace iml

#endif