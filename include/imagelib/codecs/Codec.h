#pragma once
// imagelib/codecs/Codec.h
//
// Codec abstraction. ImageLib Core never talks to a concrete codec (PNG, JPEG,
// BMP, TGA, STB, ...); everything goes through ICodec + the CodecRegistry.
//
// Ownership: codecs registered into the registry are owned by the registry
// (deleted at process shutdown). The Image class stays codec-agnostic.
//
// Thread safety: the registry is read-mostly after process startup;
// loadImage/saveImage are safe to call from multiple threads.

#include "imagelib/core/Types.h"
#include "imagelib/core/Image.h"
#include "imagelib/core/Error.h"

#include <string>
#include <vector>

namespace iml {
namespace codecs {

enum class ImageCodecFormat : uint16 {
    Unknown = 0,
    PNG,
    JPEG,
    BMP,
    TGA,
};

/// Static capabilities of a codec.
struct CodecInfo {
    ImageCodecFormat format = ImageCodecFormat::Unknown;

    /// Canonical lowercase extension WITHOUT dot (e.g. "png").
    std::string extension;

    /// Formats this codec can decode into (inputs).
    std::vector<PixelFormat> readablePixels;

    /// Formats this codec can encode from (outputs).
    std::vector<PixelFormat> writablePixels;

    bool canRead(PixelFormat pf) const noexcept {
        for (const auto& p : readablePixels) if (p == pf) return true;
        return false;
    }
    bool canWrite(PixelFormat pf) const noexcept {
        for (const auto& p : writablePixels) if (p == pf) return true;
        return false;
    }
};

/// Abstract codec: one object per file format.
class ICodec {
public:
    virtual ~ICodec() = default;
    virtual const CodecInfo& info() const = 0;

    /// Magic-byte sniffing (may return Unknown when not feasible, e.g. TGA).
    virtual bool canReadHeader(const byte* data, size_t size) const noexcept { return false; }

    /// Decode `path` into `out` (out is fully replaced).
    virtual void read(Image& out, const std::string& path) = 0;

    /// Encode inPath into `path`.
    virtual void write(const Image& src, const std::string& path,
                       const SaveOptions& opts) = 0;
};

/// Global registry. Owns registered codec instances.
class CodecRegistry {
public:
    static CodecRegistry& instance();

    ~CodecRegistry();

    /// Takes ownership of `codec`; returns false on duplicate extension.
    bool add(ICodec* codec);

    /// Finds a codec by lowercase extension.
    ICodec* findByExtension(const std::string& ext) noexcept;

    /// Finds a codec able to decode the given magic bytes.
    ICodec* findByHeader(const byte* data, size_t size) noexcept;

    size_t count() const noexcept { return codecs_.size(); }

private:
    struct Entry {
        ICodec* codec = nullptr;
    };
    std::vector<Entry> codecs_;
};

// ---------------------------------------------------------------------------
// Convenience public API
// ---------------------------------------------------------------------------

/// Detects the file format from magic bytes (best effort).
ImageCodecFormat detectFormat(const byte* data, size_t size) noexcept;

/// Lowercases + strips a dot -> "path/to/x.PNG" or "X.TGA" -> "png"/"tga".
/// Returns empty string when no extension present.
std::string extensionOf(const std::string& path);

/// Decodes a file into `out`. Registered codec is chosen by extension, with a
/// magic-byte fallback. Throws InvalidFileError / UnsupportedFormatError.
void loadImage(Image& out, const std::string& path);

/// Encodes `src` to `path`. Throws UnsupportedFormatError for unknown
/// extensions or unsupported source layouts.
void saveImage(const Image& src, const std::string& path, const SaveOptions& opts);

/// True when a codec for the given path extension is registered.
bool supportsRead(const std::string& path);
bool supportsWrite(const std::string& path);

} // namespace codecs
} // namespace iml