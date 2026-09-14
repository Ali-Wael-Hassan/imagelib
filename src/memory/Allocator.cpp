// imagelib/src/memory/Allocator.cpp
//
// Out-of-line definitions moved out of Allocator.h: the DefaultAllocator
// singleton + private header helpers and the allocation convenience free
// functions. The allocator fast paths and the Buffer layer stay in Memory.cpp.

#include "imagelib/core/memory/Allocator.h"
#include "imagelib/core/memory/Memory.h"

#include <cstddef>

namespace iml {
namespace mem {

// ---------------------------------------------------------------------------
// DefaultAllocator
// ---------------------------------------------------------------------------

DefaultAllocator& DefaultAllocator::instance() {
    static DefaultAllocator s_instance;
    return s_instance;
}

DefaultAllocator::Header* DefaultAllocator::headerOf(void* aligned) noexcept {
    return reinterpret_cast<Header*>(static_cast<byte*>(aligned) - HeaderSize);
}

const DefaultAllocator::Header* DefaultAllocator::headerOf(const void* aligned) noexcept {
    return reinterpret_cast<const Header*>(static_cast<const byte*>(aligned) - HeaderSize);
}

// ---------------------------------------------------------------------------
// Convenience free functions routing to the default allocator (no virtuals).
// ---------------------------------------------------------------------------

void* allocate(size_t size, size_t alignment) {
    return DefaultAllocator::allocateDefault(size, alignment);
}

void deallocate(void* p) noexcept {
    DefaultAllocator::deallocateDefault(p);
}

size_t allocationSize(const void* p) noexcept {
    return DefaultAllocator::allocationSizeDefault(p);
}

} // namespace mem
} // namespace iml