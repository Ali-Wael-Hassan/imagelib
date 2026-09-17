#pragma once
/// @file
/// Default malloc()-backed allocator and non-virtual free-function fast paths.
#define IMAGELIB_CORE_MEMORY_DEFAULTALLOCATOR_H_

#include "imagelib/core/memory/IAllocator.h"

#include <cstddef>
#include <cstdlib>
#include <new>

namespace iml {
namespace mem {

/// malloc()-backed, header-prefixed aligned allocation.
class DefaultAllocator final : public IAllocator {
  public:
    /// Overhead bytes stored immediately before the aligned payload.
    static constexpr size_t HeaderSize = 2 * sizeof(size_t);

    /// Allocates at least `size` bytes aligned to `alignment`.
    /// @param size Number of bytes to allocate.
    /// @param alignment Byte alignment of the returned pointer.
    /// @return Pointer to the aligned payload.
    /// @throws AllocationError On allocation failure.
    /// @throws InvalidParameterError For an invalid alignment.
    void* allocate(size_t size, size_t alignment = defaultAlignment) override;

    /// Deallocates a pointer previously returned by allocate().
    /// @param p Pointer to deallocate; nullptr is a no-op.
    void deallocate(void* p) noexcept override;

    /// Reports the requested payload size for a live pointer.
    /// @param p Pointer returned by allocate().
    /// @return Requested size in bytes, 0 when p is nullptr.
    size_t allocationSize(const void* p) const noexcept override;

    /// Non-virtual static allocation fast path.
    /// @param size Number of bytes to allocate.
    /// @param alignment Byte alignment of the returned pointer.
    /// @return Pointer to the aligned payload.
    /// @throws AllocationError On allocation failure.
    /// @throws InvalidParameterError For an invalid alignment.
    static void* allocateDefault(size_t size, size_t alignment = defaultAlignment);

    /// Non-virtual static deallocation fast path.
    /// @param p Pointer to deallocate; nullptr is a no-op.
    static void deallocateDefault(void* p) noexcept;

    /// Non-virtual static allocation-size fast path.
    /// @param p Pointer returned by allocate().
    /// @return Requested size in bytes, 0 when p is nullptr.
    static size_t allocationSizeDefault(const void* p) noexcept;

    /// Singleton process-wide default allocator.
    /// @return Reference to the singleton instance.
    static DefaultAllocator& instance();

  private:
    /// Header stored immediately before the aligned payload.
    struct Header {
        /// Raw block returned by std::malloc.
        void* base;
        /// Requested payload size.
        size_t size;
    };

    /// Returns the header preceding an aligned payload.
    /// @param aligned Aligned payload pointer.
    /// @return Pointer to its header.
    static Header* headerOf(void* aligned) noexcept;

    /// Returns the header preceding an aligned payload (const).
    /// @param aligned Aligned payload pointer.
    /// @return Const pointer to its header.
    static const Header* headerOf(const void* aligned) noexcept;
};

/// Allocates at least `size` bytes from the default allocator.
/// @param size Number of bytes to allocate.
/// @param alignment Byte alignment of the returned pointer.
/// @return Pointer to the aligned payload.
/// @throws AllocationError On allocation failure.
/// @throws InvalidParameterError For an invalid alignment.
void* allocate(size_t size, size_t alignment = defaultAlignment);

/// Deallocates a default-allocated pointer (no-op for nullptr).
/// @param p Pointer to deallocate.
void deallocate(void* p) noexcept;

/// Reports the requested size of a default-allocated block (0 for nullptr).
/// @param p Pointer returned by allocate().
/// @return Requested size in bytes, 0 when p is nullptr.
size_t allocationSize(const void* p) noexcept;

} // namespace mem
} // namespace iml