#pragma once
// imagelib/codecs/StbCodec.h
//
// STB-based codec backend (PNG/JPEG/BMP/TGA). This is the ONLY place in
// ImageLib that touches stb_image. The STB implementation macros live in
// external/stb_image/src/*.cpp, never in public headers.

namespace iml {
namespace codecs {

/// Registers the STB-backed codecs into the global registry (idempotent).
/// Called automatically by the static initializer in StbCodec.cpp; exported
/// for completeness if a user constructs a bespoke registry.
void registerStbCodec();

} // namespace codecs
} // namespace iml