#ifndef IMAGELIB_CODECS_STB_H_
#define IMAGELIB_CODECS_STB_H_
#pragma once
/// @file Stb.h
/// STB-based codec backend (PNG/JPEG/BMP/TGA).

namespace iml {
namespace codecs {

/// Registers the STB-backed codecs into the global registry (idempotent).
/// Called automatically by the static initializer in Stb.cpp; exported for
/// completeness if a user constructs a bespoke registry.
void registerStbCodec();

} // namespace codecs
} // namespace iml

#endif