#include "imagelib/core/memory/Memory.h"

#include <cstring>

namespace iml {
namespace mem {

/// Rounds a pointer up to a multiple of `alignment`.
/// @param p Pointer to align.
/// @param alignment Power-of-two alignment.
/// @return The aligned pointer.
const void* alignUpPtr(const void* p, size_t alignment) noexcept {
    const uintptr_t v = reinterpret_cast<uintptr_t>(p);
    return reinterpret_cast<const void*>((v + (alignment - 1)) & ~(alignment - 1));
}

/// Rounds a pointer up to a multiple of `alignment` (non-const).
/// @param p Pointer to align.
/// @param alignment Power-of-two alignment.
/// @return The aligned pointer.
void* alignUpPtr(void* p, size_t alignment) noexcept {
    return const_cast<void*>(alignUpPtr(static_cast<const void*>(p), alignment));
}

/// Overflow-checked add.
/// @param a First operand.
/// @param b Second operand.
/// @return The sum `a + b`.
/// @throws IntegerOverflowError On wrap-around.
size_t addChecked(size_t a, size_t b) {
    if (wouldAddOverflow(a, b)) {
        throw IntegerOverflowError("mem::addChecked: size overflow");
    }
    return a + b;
}

/// Overflow-checked multiply.
/// @param a First operand.
/// @param b Second operand.
/// @return The product `a * b`.
/// @throws IntegerOverflowError On wrap-around.
size_t mulChecked(size_t a, size_t b) {
    if (wouldMulOverflow(a, b)) {
        throw IntegerOverflowError("mem::mulChecked: size overflow");
    }
    return a * b;
}

/// Overflow-checked multiply of three factors.
/// @param a First operand.
/// @param b Second operand.
/// @param c Third factor.
/// @return The product `a * b * c`.
/// @throws IntegerOverflowError On wrap-around.
size_t mulChecked3(size_t a, size_t b, size_t c) { return mulChecked(mulChecked(a, b), c); }

} // namespace mem
} // namespace iml