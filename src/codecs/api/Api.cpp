#include "imagelib/codecs/api/Api.h"
#include "imagelib/codecs/registry/CodecRegistry.h"
#include "imagelib/codecs/stb/Stb.h"
#include "imagelib/codecs/CodecUtils.h"

#include <cstdio>

namespace iml {
namespace codecs {

namespace {

/// Pulls the STB backend object file out of the static library. Without a
/// direct symbol reference, linkers drop Stb.o and with it its static
/// registration. Safe to call repeatedly.
/// @return Always true once registration has been performed.
bool ensureStbCodecsRegistered() {
    static const bool registered = (registerStbCodec(), true);
    (void)registered;
    return registered;
}

} // namespace

/// Detects the file format from magic bytes (best effort).
/// @param data Pointer to the file's leading bytes.
/// @param size Number of available bytes.
/// @return Detected format, or Unknown when unrecognized.
ImageCodecFormat detectFormat(const byte* data, size_t size) noexcept {
    if (data == nullptr)
        return ImageCodecFormat::Unknown;
    static const ImageCodecFormat kPng = ImageCodecFormat::PNG;
    static const ImageCodecFormat kJpeg = ImageCodecFormat::JPEG;
    static const ImageCodecFormat kBmp = ImageCodecFormat::BMP;
    if (size >= 8 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
        return kPng;
    if (size >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
        return kJpeg;
    if (size >= 2 && data[0] == 'B' && data[1] == 'M')
        return kBmp;
    return ImageCodecFormat::Unknown;
}

/// Lowercases and strips the leading dot from a path extension.
/// @param path File path.
/// @return Lower-cased extension without the dot, or empty when none present.
std::string extensionOf(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return std::string();
    return detail::toLowerASCII(path.substr(dot + 1));
}

/// Decodes a file into `out` via the registry.
/// @param out Image that receives the decoded pixels (fully replaced).
/// @param path Path of the file to decode.
/// @throws InvalidFileError When the file cannot be opened or read.
/// @throws UnsupportedFormatError When no matching codec is registered.
void loadImage(Image& out, const std::string& path) {
    ensureStbCodecsRegistered();
    std::string ext = extensionOf(path);
    ICodec* codec = ext.empty() ? nullptr : CodecRegistry::instance().findByExtension(ext);

    if (codec == nullptr) {
        byte head[16];
        std::FILE* f = std::fopen(path.c_str(), "rb");
        if (f == nullptr)
            throw InvalidFileError("codecs::loadImage: cannot open file: " + path);
        size_t n = std::fread(head, 1, sizeof(head), f);
        std::fclose(f);
        if (n == 0)
            throw InvalidFileError("codecs::loadImage: empty file: " + path);
        codec = CodecRegistry::instance().findByHeader(head, n);
        if (codec == nullptr)
            throw UnsupportedFormatError("codecs::loadImage: unknown format: " + path);
    }

    codec->read(out, path);
}

/// Encodes `src` to `path` via the registry.
/// @param src Image to encode.
/// @param path Destination file path (extension selects the codec).
/// @param opts Save options to pass to the codec.
/// @throws UnsupportedFormatError For unknown extensions or unsupported source layouts.
void saveImage(const Image& src, const std::string& path, const SaveOptions& opts) {
    ensureStbCodecsRegistered();
    std::string ext = extensionOf(path);
    ICodec* codec = CodecRegistry::instance().findByExtension(ext);
    if (codec == nullptr)
        throw UnsupportedFormatError("codecs::saveImage: unsupported extension for: " + path);
    codec->write(src, path, opts);
}

/// True when a codec for the given path extension is registered.
/// @param path File path.
/// @return True when the extension can be decoded.
bool supportsRead(const std::string& path) {
    ensureStbCodecsRegistered();
    std::string ext = extensionOf(path);
    return !ext.empty() && CodecRegistry::instance().findByExtension(ext) != nullptr;
}
/// True when a codec for the given path extension can encode files.
/// @param path File path.
/// @return True when the extension can be encoded.
bool supportsWrite(const std::string& path) { return supportsRead(path); }

} // namespace codecs
} // namespace iml