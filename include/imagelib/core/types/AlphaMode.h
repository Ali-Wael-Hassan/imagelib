#pragma once
#ifndef IMAGELIB_CORE_ALPHAMODE_H_
#define IMAGELIB_CORE_ALPHAMODE_H_
/// @file
/// Alpha channel mode enumeration.

#include "imagelib/core/types/FixedTypes.h"

namespace iml {

/// How alpha is represented relative to color samples.
enum class AlphaMode : uint8 {
    /// Alpha mode is not known.
    Unknown = 0,
    /// No alpha channel present.
    None,
    /// Color is not multiplied by alpha.
    Straight,
    /// Color is already multiplied by alpha.
    Premultiplied,
};

} // namespace iml

#endif