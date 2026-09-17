#ifndef IMAGELIB_CODECS_API_H_
#define IMAGELIB_CODECS_API_H_
#pragma once
/// @file Api.h
/// Convenience public API: format detection, extension parsing, and the
/// thread-safe load/save entry points.

#include "imagelib/codecs/interface/Format.h"
#include "imagelib/core/image/Image.h"

#include <string>

namespace iml {
namespace codecs {

/// Detects the file format from magic bytes (best effort).
/// @param data Pointer to the file's leading bytes.
/// @param size Number of available bytes.
/// @return Detected format, or Unknown when unrecognized.
ImageCodecFormat detectFormat(const byte* data, size_t size) noexcept;

/// Lowercases + strips a dot -> "path/to/x.PNG" or "X.TGA" -> "png"/"tga".
/// @param path File path.
/// @return Lower-cased extension without the dot, or empty when none present.
std::string extensionOf(const std::string& path);

/// Decodes a file into `out`. Registered codec is chosen by extension, with a
/// magic-byte fallback.
/// @param out Image that receives the decoded pixels (fully replaced).
/// @param path Path of the file to decode.
/// @throws InvalidFileError When the file cannot be opened or read.
/// @throws UnsupportedFormatError When no matching codec is registered.
void loadImage(Image& out, const std::string& path);

/// Encodes `src` to `path`.
/// @param src Image to encode.
/// @param path Destination file path (extension selects the codec).
/// @param opts Save options to pass to the codec.
/// @throws UnsupportedFormatError For unknown extensions or unsupported source layouts.
void saveImage(const Image& src, const std::string& path, const SaveOptions& opts);

/// True when a codec for the given path extension is registered.
/// @param path File path.
/// @return True when the extension can be decoded.
bool supportsRead(const std::string& path);
/// True when a codec for the given path extension can encode files.
/// @param path File path.
/// @return True when the extension can be encoded.
bool supportsWrite(const std::string& path);

} // namespace codecs
} // namespace iml

#endif