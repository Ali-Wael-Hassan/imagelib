#ifndef IMAGELIB_SRC_CODECS_CODECUTILS_H_
#define IMAGELIB_SRC_CODECS_CODECUTILS_H_
#pragma once
/// @file CodecUtils.h
/// Internal helpers shared by the codec registry and public API.

#include <algorithm>
#include <cctype>
#include <string>

namespace iml {
namespace codecs {
namespace detail {

/// Lowercases all ASCII characters in \p s.
/// @param s String to lower-case.
/// @return Lower-cased copy of \p s.
inline std::string toLowerASCII(const std::string& s) {
    std::string r(s);
    std::transform(r.begin(), r.end(), r.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return r;
}

} // namespace detail
} // namespace codecs
} // namespace iml

#endif