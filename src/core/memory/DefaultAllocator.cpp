#include "imagelib/core/memory/Allocator.h"
#include "imagelib/core/memory/Memory.h"

#include <cstddef>

namespace iml {
namespace mem {

/// Returns the process-wide singleton default allocator.
/// @return Reference to the singleton instance.
DefaultAllocator& DefaultAllocator::instance() {
    static DefaultAllocator s_instance;
    return s_instance;
}

/// Returns the header preceding an aligned payload.
/// @param aligned Aligned payload pointer.
/// @return Pointer to its header.
DefaultAllocator::Header* DefaultAllocator::headerOf(void* aligned) noexcept {
    return reinterpret_cast<Header*>(static_cast<byte*>(aligned) - HeaderSize);
}

/// Returns the header preceding an aligned payload (const).
/// @param aligned Aligned payload pointer.
/// @return Const pointer to its header.
const DefaultAllocator::Header* DefaultAllocator::headerOf(const void* aligned) noexcept {
    return reinterpret_cast<const Header*>(static_cast<const byte*>(aligned) - HeaderSize);
}

/// Virtual allocate() forwarding to the non-virtual fast path.
/// @param size Number of bytes to allocate.
/// @param alignment Byte alignment of the returned pointer.
/// @return Pointer to the aligned payload.
/// @throws AllocationError On allocation failure.
/// @throws InvalidParameterError For an invalid alignment.
void* DefaultAllocator::allocate(size_t size, size_t alignment) {
    return allocateDefault(size, alignment);
}

/// Allocates at least `size` bytes aligned to `alignment`.
/// @param size Number of bytes to allocate.
/// @param alignment Byte alignment of the returned pointer.
/// @return Pointer to the aligned payload.
/// @throws AllocationError On allocation failure.
/// @throws InvalidParameterError For an invalid alignment.
void* DefaultAllocator::allocateDefault(size_t size, size_t alignment) {
    if (!isValidAlignment(alignment)) {
        throw InvalidParameterError("DefaultAllocator: invalid alignment");
    }

    const size_t total = addChecked(addChecked(HeaderSize, size), alignment - 1);
    void* raw = std::malloc(total);
    if (raw == nullptr) {
        throw AllocationError("DefaultAllocator: malloc returned nullptr");
    }

    byte* aligned = static_cast<byte*>(alignUpPtr(static_cast<byte*>(raw) + HeaderSize, alignment));
    Header* h = reinterpret_cast<Header*>(aligned - HeaderSize);
    h->base = raw;
    h->size = size;

    return aligned;
}

/// Virtual deallocate() forwarding to the non-virtual fast path.
/// @param p Pointer to deallocate; nullptr is a no-op.
void DefaultAllocator::deallocate(void* p) noexcept { deallocateDefault(p); }

/// Frees a pointer returned by allocateDefault().
/// @param p Pointer to deallocate; nullptr is a no-op.
void DefaultAllocator::deallocateDefault(void* p) noexcept {
    if (p == nullptr)
        return;
    Header* h = headerOf(p);
    std::free(h->base);
}

/// Virtual allocationSize() forwarding to the non-virtual fast path.
/// @param p Pointer returned by allocate().
/// @return Requested size in bytes, 0 when p is nullptr.
size_t DefaultAllocator::allocationSize(const void* p) const noexcept {
    return allocationSizeDefault(p);
}

/// Reports the requested payload size for a live pointer.
/// @param p Pointer returned by allocate().
/// @return Requested size in bytes, 0 when p is nullptr.
size_t DefaultAllocator::allocationSizeDefault(const void* p) noexcept {
    if (p == nullptr)
        return 0;
    return headerOf(p)->size;
}

/// Allocates at least `size` bytes from the default allocator.
/// @param size Number of bytes to allocate.
/// @param alignment Byte alignment of the returned pointer.
/// @return Pointer to the aligned payload.
/// @throws AllocationError On allocation failure.
/// @throws InvalidParameterError For an invalid alignment.
void* allocate(size_t size, size_t alignment) {
    return DefaultAllocator::allocateDefault(size, alignment);
}

/// Deallocates a default-allocated pointer (no-op for nullptr).
/// @param p Pointer to deallocate.
void deallocate(void* p) noexcept { DefaultAllocator::deallocateDefault(p); }

/// Reports the requested size of a default-allocated block (0 for nullptr).
/// @param p Pointer returned by allocate().
/// @return Requested size in bytes, 0 when p is nullptr.
size_t allocationSize(const void* p) noexcept { return DefaultAllocator::allocationSizeDefault(p); }

} // namespace mem
} // namespace iml