#pragma once
#define IMAGELIB_CORE_MEMORY_MEMORY_H_
// imagelib/core/memory/Memory.h
//
// Low-level memory primitives:
//   - alignment helpers (SSE/AVX/cache-line friendly)
//   - overflow-safe size arithmetic
//   - raw memory utilities (copy/move/set/zero/compare)
//
// These are the foundation for every allocation performed by ImageLib and
// are deliberately free of any dependency on the image system.

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"

#include <cstddef>
#include <cstring>
#include <cstdint>
#include <limits>

namespace iml {
namespace mem {

// ---------------------------------------------------------------------------
// Alignment constants / utilities.
// ---------------------------------------------------------------------------

constexpr size_t cacheLineSize = 64;      ///< Typical x86/ARM cache line.
constexpr size_t pageSize       = 4096;   ///< Common OS page size.
constexpr size_t defaultAlignment = 16;   ///< Safe default (SSE-16B loads).
constexpr size_t simdAlignment    = 64;   ///< AVX-512 / cache-line friendly.
constexpr size_t maxAlignment     = 4096; ///< Upper bound for sanity checks.

/// True when p is aligned to `alignment` bytes.
constexpr bool isAligned(const void* p, size_t alignment) noexcept {
    return (reinterpret_cast<uintptr_t>(p) & (alignment - 1)) == 0;
}

/// Round v up to a multiple of alignment (alignment must be a power of two).
constexpr size_t alignUp(size_t v, size_t alignment) noexcept {
    return (v + (alignment - 1)) & ~(alignment - 1);
}

/// Round v down to a multiple of alignment.
constexpr size_t alignDown(size_t v, size_t alignment) noexcept {
    return v & ~(alignment - 1);
}

/// Pointer variant of alignUp.
const void* alignUpPtr(const void* p, size_t alignment) noexcept;
void* alignUpPtr(void* p, size_t alignment) noexcept;

/// Validates that an alignment is a supported power of two.
constexpr bool isValidAlignment(size_t alignment) noexcept {
    return alignment != 0 && (alignment & (alignment - 1)) == 0
        && alignment <= maxAlignment;
}

// ---------------------------------------------------------------------------
// Overflow-safe size arithmetic.
//
// ImageLib uses 32-bit image dimensions but buffers may exceed 4 GiB for
// large multi-channel images; every size computation goes through these
// helpers so a pair of malicious/accidental dimensions fails loudly instead
// of silently wrapping.
// ---------------------------------------------------------------------------

constexpr bool wouldAddOverflow(size_t a, size_t b) noexcept {
    return a > std::numeric_limits<size_t>::max() - b;
}
constexpr bool wouldMulOverflow(size_t a, size_t b) noexcept {
    if (a == 0 || b == 0) return false;
    return a > std::numeric_limits<size_t>::max() / b;
}

/// Overflow-checked add; throws IntegerOverflowError on wrap.
size_t addChecked(size_t a, size_t b);

/// Overflow-checked multiply.
size_t mulChecked(size_t a, size_t b);

/// Overflow-checked multiply of three factors.
size_t mulChecked3(size_t a, size_t b, size_t c);

// ---------------------------------------------------------------------------
// Raw memory utilities.
//
// Thin wrappers today; the architecture allows dropping in dedicated SIMD /
// non-temporal / hardware-accelerated implementations later without touching
// callers.
// ---------------------------------------------------------------------------

void copy(void* dst, const void* src, size_t bytes) noexcept;

/// Overlapping-safe copy.
void move_(void* dst, const void* src, size_t bytes) noexcept;

void set(void* dst, uint8 value, size_t bytes) noexcept;

void zero(void* dst, size_t bytes) noexcept;

/// Lexicographic byte comparison, memcmp semantics: <0, 0, >0.
int compare(const void* a, const void* b, size_t bytes) noexcept;

} // namespace mem
} // namespace iml