#include "imagelib/codecs/registry/CodecRegistry.h"
#include "imagelib/codecs/CodecUtils.h"

namespace iml {
namespace codecs {

/// Returns the process-wide registry singleton.
CodecRegistry& CodecRegistry::instance() {
    static CodecRegistry s_instance;
    return s_instance;
}

/// Frees all registered codec instances.
CodecRegistry::~CodecRegistry() {
    for (auto& c : codecs_)
        delete c;
    codecs_.clear();
}

/// Takes ownership of `codec`.
/// @param codec Codec to register; ownership transfers on success.
/// @return False on duplicate extension or invalid input.
bool CodecRegistry::add(ICodec* codec) {
    if (codec == nullptr)
        return false;
    std::string ext = detail::toLowerASCII(codec->info().extension);
    if (ext.empty())
        return false;
    for (const auto& c : codecs_) {
        if (detail::toLowerASCII(c->info().extension) == ext)
            return false;
    }
    codecs_.push_back(codec);
    return true;
}

/// Finds a codec by lowercase extension.
/// @param ext File extension (case-insensitive).
/// @return Matching codec, or nullptr.
ICodec* CodecRegistry::findByExtension(const std::string& ext) noexcept {
    std::string e = detail::toLowerASCII(ext);
    for (auto& c : codecs_) {
        if (c->info().extension == e)
            return c;
    }
    return nullptr;
}

/// Finds a codec able to decode the given magic bytes.
/// @param data Pointer to the file's leading bytes.
/// @param size Number of available bytes.
/// @return Matching codec, or nullptr.
ICodec* CodecRegistry::findByHeader(const byte* data, size_t size) noexcept {
    if (data == nullptr || size == 0)
        return nullptr;
    for (auto& c : codecs_) {
        if (c->canReadHeader(data, size))
            return c;
    }
    return nullptr;
}

} // namespace codecs
} // namespace iml