#pragma once
/// @file
/// Abstract allocation interface (customization point for allocators).
#define IMAGELIB_CORE_MEMORY_IALLOCATOR_H_

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/core/memory/Memory.h"

#include <cstddef>

namespace iml {
namespace mem {

/// Abstract allocation interface implemented by allocators.
class IAllocator {
  public:
    /// Virtual destructor.
    virtual ~IAllocator() = default;

    /// Allocates at least `size` bytes aligned to `alignment`.
    /// @param size Number of bytes to allocate.
    /// @param alignment Byte alignment of the returned pointer.
    /// @return Pointer to the allocated block.
    /// @throws AllocationError On allocation failure.
    /// @throws InvalidParameterError For an invalid alignment.
    virtual void* allocate(size_t size, size_t alignment = defaultAlignment) = 0;

    /// Deallocates a pointer returned by allocate(); must pair with the same
    /// allocator used for allocation.
    /// @param p Pointer to deallocate; nullptr is a no-op.
    virtual void deallocate(void* p) noexcept = 0;

    /// Reports the requested payload size for a live pointer.
    /// @param p Pointer returned by allocate().
    /// @return Requested size in bytes, 0 for nullptr.
    virtual size_t allocationSize(const void* p) const noexcept = 0;
};

} // namespace mem
} // namespace iml