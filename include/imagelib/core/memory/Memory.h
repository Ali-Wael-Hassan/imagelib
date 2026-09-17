#pragma once
/// @file
/// Low-level memory primitives and overflow-safe size arithmetic.
#define IMAGELIB_CORE_MEMORY_MEMORY_H_

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"

#include <cstddef>
#include <cstring>
#include <cstdint>
#include <limits>

namespace iml {
namespace mem {

constexpr size_t cacheLineSize = 64;    ///< Typical x86/ARM cache line.
constexpr size_t pageSize = 4096;       ///< Common OS page size.
constexpr size_t defaultAlignment = 16; ///< Safe default (SSE-16B loads).
constexpr size_t simdAlignment = 64;    ///< AVX-512 / cache-line friendly.
constexpr size_t maxAlignment = 4096;   ///< Upper bound for sanity checks.

/// True when p is aligned to `alignment` bytes.
/// @param p Pointer to check.
/// @param alignment Required alignment in bytes.
/// @return True when the pointer is aligned.
constexpr bool isAligned(const void* p, size_t alignment) noexcept {
    return (reinterpret_cast<uintptr_t>(p) & (alignment - 1)) == 0;
}

/// Round v up to a multiple of alignment (alignment must be a power of two).
/// @param v Value to round.
/// @param alignment Power-of-two alignment.
/// @return The rounded-up value.
constexpr size_t alignUp(size_t v, size_t alignment) noexcept {
    return (v + (alignment - 1)) & ~(alignment - 1);
}

/// Round v down to a multiple of alignment.
/// @param v Value to round.
/// @param alignment Power-of-two alignment.
/// @return The rounded-down value.
constexpr size_t alignDown(size_t v, size_t alignment) noexcept { return v & ~(alignment - 1); }

/// Pointer variant of alignUp.
/// @param p Pointer to align.
/// @param alignment Power-of-two alignment.
/// @return The aligned pointer.
const void* alignUpPtr(const void* p, size_t alignment) noexcept;

/// Non-const pointer variant of alignUp.
/// @param p Pointer to align.
/// @param alignment Power-of-two alignment.
/// @return The aligned pointer.
void* alignUpPtr(void* p, size_t alignment) noexcept;

/// Validates that an alignment is a supported power of two.
/// @param alignment Alignment to validate.
/// @return True when alignment is a supported power of two within maxAlignment.
constexpr bool isValidAlignment(size_t alignment) noexcept {
    return alignment != 0 && (alignment & (alignment - 1)) == 0 && alignment <= maxAlignment;
}

/// Returns whether `a + b` would overflow size_t.
/// @param a First operand.
/// @param b Second operand.
/// @return True when the addition would wrap.
constexpr bool wouldAddOverflow(size_t a, size_t b) noexcept {
    return a > std::numeric_limits<size_t>::max() - b;
}

/// Returns whether `a * b` would overflow size_t.
/// @param a First operand.
/// @param b Second operand.
/// @return True when the product would wrap.
constexpr bool wouldMulOverflow(size_t a, size_t b) noexcept {
    if (a == 0 || b == 0)
        return false;
    return a > std::numeric_limits<size_t>::max() / b;
}

/// Overflow-checked add.
/// @param a First operand.
/// @param b Second operand.
/// @return The sum `a + b`.
/// @throws IntegerOverflowError On wrap-around.
size_t addChecked(size_t a, size_t b);

/// Overflow-checked multiply.
/// @param a First operand.
/// @param b Second operand.
/// @return The product `a * b`.
/// @throws IntegerOverflowError On wrap-around.
size_t mulChecked(size_t a, size_t b);

/// Overflow-checked multiply of three factors.
/// @param a First operand.
/// @param b Second operand.
/// @param c Third operand.
/// @return The product `a * b * c`.
/// @throws IntegerOverflowError On wrap-around.
size_t mulChecked3(size_t a, size_t b, size_t c);

/// Copies `bytes` bytes from `src` to `dst`.
/// @param dst Destination pointer.
/// @param src Source pointer.
/// @param bytes Number of bytes to copy.
inline void copy(void* dst, const void* src, size_t bytes) noexcept {
    if (dst == nullptr || src == nullptr || bytes == 0)
        return;
    std::memcpy(dst, src, bytes);
}

/// Overlapping-safe copy.
/// @param dst Destination pointer.
/// @param src Source pointer.
/// @param bytes Number of bytes to copy.
inline void move_(void* dst, const void* src, size_t bytes) noexcept {
    if (dst == nullptr || src == nullptr || bytes == 0)
        return;
    std::memmove(dst, src, bytes);
}

/// Fills `bytes` bytes at `dst` with `value`.
/// @param dst Destination pointer.
/// @param value Byte value to write.
/// @param bytes Number of bytes to fill.
inline void set(void* dst, uint8 value, size_t bytes) noexcept {
    if (dst == nullptr || bytes == 0)
        return;
    std::memset(dst, value, bytes);
}

/// Fills `bytes` bytes at `dst` with zero.
/// @param dst Destination pointer.
/// @param bytes Number of bytes to zero.
inline void zero(void* dst, size_t bytes) noexcept {
    if (dst == nullptr || bytes == 0)
        return;
    std::memset(dst, 0, bytes);
}

/// Lexicographic byte comparison (memcmp semantics).
/// @param a First block.
/// @param b Second block.
/// @param bytes Number of bytes to compare.
inline int compare(const void* a, const void* b, size_t bytes) noexcept {
    return std::memcmp(a, b, bytes);
}

} // namespace mem
} // namespace iml