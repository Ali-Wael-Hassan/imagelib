// imagelib/src/codecs/Codec.cpp
//
// Codec registry, format detection, and the public thread-safe load/save
// entry points.

#include "imagelib/codecs/Codec.h"
#include "imagelib/codecs/StbCodec.h"

#include <algorithm>
#include <cctype>
#include <memory>

namespace iml {
namespace codecs {

namespace {

std::string toLowerASCII(const std::string& s) {
    std::string r(s);
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return r;
}

/// Pulls the STB backend object file out of the static library. Without a
/// direct symbol reference, linkers drop StbCodec.o and with it its static
/// registration. Safe to call repeatedly.
bool ensureStbCodecsRegistered() {
    static const bool registered = (registerStbCodec(), true);
    (void)registered;
    return registered;
}

} // namespace

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------

CodecRegistry& CodecRegistry::instance() {
    static CodecRegistry s_instance;
    return s_instance;
}

CodecRegistry::~CodecRegistry() {
    for (auto& e : codecs_) delete e.codec;
    codecs_.clear();
}

bool CodecRegistry::add(ICodec* codec) {
    if (codec == nullptr) return false;
    std::string ext = toLowerASCII(codec->info().extension);
    if (ext.empty()) return false;
    for (const auto& e : codecs_) {
        if (e.codec->info().extension == ext) return false;
    }
    codecs_.push_back({ codec });
    return true;
}

ICodec* CodecRegistry::findByExtension(const std::string& ext) noexcept {
    std::string e = toLowerASCII(ext);
    for (auto& entry : codecs_) {
        if (entry.codec->info().extension == e) return entry.codec;
    }
    return nullptr;
}

ICodec* CodecRegistry::findByHeader(const byte* data, size_t size) noexcept {
    if (data == nullptr || size == 0) return nullptr;
    for (auto& entry : codecs_) {
        if (entry.codec->canReadHeader(data, size)) return entry.codec;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Detection helpers
// ---------------------------------------------------------------------------

ImageCodecFormat detectFormat(const byte* data, size_t size) noexcept {
    if (data == nullptr) return ImageCodecFormat::Unknown;
    if (size >= 8 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N'
        && data[3] == 'G') return ImageCodecFormat::PNG;
    if (size >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
        return ImageCodecFormat::JPEG;
    if (size >= 2 && data[0] == 'B' && data[1] == 'M') return ImageCodecFormat::BMP;
    return ImageCodecFormat::Unknown; // TGA has no reliable magic
}

std::string extensionOf(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return std::string();
    return toLowerASCII(path.substr(dot + 1));
}

// ---------------------------------------------------------------------------
// Public entry points
// ---------------------------------------------------------------------------

void loadImage(Image& out, const std::string& path) {
    ensureStbCodecsRegistered();
    std::string ext = extensionOf(path);
    ICodec* codec = ext.empty() ? nullptr : CodecRegistry::instance().findByExtension(ext);

    if (codec == nullptr) {
        // Fall back to magic-byte sniffing.
        byte head[16];
        std::FILE* f = std::fopen(path.c_str(), "rb");
        if (f == nullptr) {
            throw InvalidFileError("codecs::loadImage: cannot open file: " + path);
        }
        size_t n = std::fread(head, 1, sizeof(head), f);
        std::fclose(f);
        if (n == 0) throw InvalidFileError("codecs::loadImage: empty file: " + path);
        codec = CodecRegistry::instance().findByHeader(head, n);
        if (codec == nullptr) {
            throw UnsupportedFormatError("codecs::loadImage: unknown format: " + path);
        }
    }

    codec->read(out, path);
}

void saveImage(const Image& src, const std::string& path, const SaveOptions& opts) {
    ensureStbCodecsRegistered();
    std::string ext = extensionOf(path);
    ICodec* codec = CodecRegistry::instance().findByExtension(ext);
    if (codec == nullptr) {
        throw UnsupportedFormatError("codecs::saveImage: unsupported extension for: " + path);
    }
    codec->write(src, path, opts);
}

bool supportsRead(const std::string& path) {
    ensureStbCodecsRegistered();
    std::string ext = extensionOf(path);
    return !ext.empty() && CodecRegistry::instance().findByExtension(ext) != nullptr;
}
bool supportsWrite(const std::string& path) {
    return supportsRead(path);
}

} // namespace codecs
} // namespace iml