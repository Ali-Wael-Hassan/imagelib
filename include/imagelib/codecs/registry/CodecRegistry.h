#ifndef IMAGELIB_CODECS_CODECREGISTRY_H_
#define IMAGELIB_CODECS_CODECREGISTRY_H_
#pragma once
/// @file CodecRegistry.h
/// Global codec registry that owns registered codec instances.

#include "imagelib/codecs/interface/ICodec.h"

#include <string>
#include <vector>

namespace iml {
namespace codecs {

/// Global registry. Owns registered codec instances.
class CodecRegistry {
  public:
    /// Returns the process-wide registry singleton.
    static CodecRegistry& instance();

    /// Frees all registered codecs.
    ~CodecRegistry();

    /// Takes ownership of `codec`.
    /// @param codec Codec to register; ownership transfers on success.
    /// @return False on duplicate extension.
    bool add(ICodec* codec);

    /// Finds a codec by lowercase extension.
    /// @param ext File extension (case-insensitive).
    /// @return Matching codec, or nullptr.
    ICodec* findByExtension(const std::string& ext) noexcept;

    /// Finds a codec able to decode the given magic bytes.
    /// @param data Pointer to the file's leading bytes.
    /// @param size Number of available bytes.
    /// @return Matching codec, or nullptr.
    ICodec* findByHeader(const byte* data, size_t size) noexcept;

    /// Number of registered codecs.
    size_t count() const noexcept { return codecs_.size(); }

  private:
    std::vector<ICodec*> codecs_;
};

} // namespace codecs
} // namespace iml

#endif