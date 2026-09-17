#pragma once
/// @file
/// Owning raw-memory buffer abstraction built on the allocator layer.
#define IMAGELIB_CORE_MEMORY_BUFFER_H_

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/core/memory/Memory.h"
#include "imagelib/core/memory/Allocator.h"

#include <cstddef>

namespace iml {
namespace mem {

/// Owning raw-memory buffer that is deep-copyable and movable.
class Buffer {
  public:
    /// Empty buffer marked for the default allocator.
    Buffer() noexcept = default;

    /// Creates a buffer of `size` bytes aligned to `alignment` using `alloc`.
    ///
    /// @param size Number of bytes to allocate.
    /// @param alignment Byte alignment of the allocation.
    /// @param alloc Allocator used to acquire memory.
    /// @throws AllocationError On allocation failure.
    /// @throws IntegerOverflowError On size computation overflow.
    explicit Buffer(
        size_t size,
        size_t alignment = defaultAlignment,
        IAllocator* alloc = &DefaultAllocator::instance());

    /// Releases the owned memory through the owning allocator.
    ~Buffer();

    /// Creates a deep copy of `other`.
    /// @param other Buffer to copy from.
    /// @throws AllocationError On allocation failure.
    Buffer(const Buffer& other);

    /// Copy-assigns from `other`.
    /// @param other Buffer to copy from.
    /// @return Reference to this buffer.
    /// @throws AllocationError On allocation failure.
    Buffer& operator=(const Buffer& other);

    /// Transfers ownership from `other`.
    /// @param other Buffer to take ownership from (left empty).
    Buffer(Buffer&& other) noexcept;

    /// Move-assigns ownership from `other`.
    /// @param other Buffer to take ownership from (left empty).
    /// @return Reference to this buffer.
    Buffer& operator=(Buffer&& other) noexcept;

    /// Ensures capacity >= newCapacity; copies contents on regrow.
    /// @param newCapacity Required capacity.
    /// @throws AllocationError On allocation failure.
    void reserve(size_t newCapacity);

    /// Changes logical size; grows (zero-filled new tail) or truncates.
    /// @param newSize New logical size.
    /// @throws AllocationError On allocation failure.
    void resize(size_t newSize);

    /// Logical size -> 0 (keeps capacity).
    void clear() noexcept;

    /// Returns a pointer to the first byte.
    /// @return Pointer to the owned data (nullptr when empty).
    byte* data() noexcept;

    /// Returns a const pointer to the first byte.
    /// @return Const pointer to the owned data (nullptr when empty).
    const byte* data() const noexcept;

    /// Returns a pointer to the first byte.
    /// @return Pointer to the first byte.
    byte* begin() noexcept;

    /// Returns a pointer one past the last byte.
    /// @return Pointer one past the end.
    byte* end() noexcept;

    /// Returns a const pointer to the first byte.
    /// @return Const pointer to the first byte.
    const byte* begin() const noexcept;

    /// Returns a const pointer one past the last byte.
    /// @return Const pointer one past the end.
    const byte* end() const noexcept;

    /// Accesses the byte at index `i`.
    /// @param i Zero-based byte index.
    /// @return Reference to the byte at `i`.
    byte& operator[](size_t i) noexcept;

    /// Accesses the byte at index `i` (const).
    /// @param i Zero-based byte index.
    /// @return Const reference to the byte at `i`.
    const byte& operator[](size_t i) const noexcept;

    /// Returns the logical size in bytes.
    /// @return Number of bytes in the buffer.
    size_t size() const noexcept;

    /// Returns whether the buffer is empty.
    /// @return True when size() == 0.
    bool empty() const noexcept;

    /// Returns the allocated capacity in bytes.
    /// @return Allocated capacity in bytes.
    size_t capacity() const noexcept;

    /// Returns the allocation alignment in bytes.
    /// @return Alignment of the owned allocation.
    size_t alignment() const noexcept;

    /// Returns the allocator used for this buffer.
    /// @return Pointer to the owning allocator.
    IAllocator* allocator() const noexcept;

    /// Exchanges contents with `other`.
    /// @param other Buffer to swap with.
    void swap(Buffer& other) noexcept;

  private:
    /// Frees the owned memory and resets the buffer to empty.
    void release() noexcept;

    /// Grows capacity to `newCapacity` preserving contents.
    /// @param newCapacity Target capacity.
    /// @throws AllocationError On allocation failure.
    void regrow(size_t newCapacity);

    /// Allocator used for this buffer's memory.
    IAllocator* alloc_ = &DefaultAllocator::instance();
    /// Pointer to the owned data block.
    byte* data_ = nullptr;
    /// Logical size in bytes.
    size_t size_ = 0;
    /// Allocated capacity in bytes.
    size_t capacity_ = 0;
    /// Allocation alignment in bytes.
    size_t alignment_ = defaultAlignment;
};

/// Exchanges contents of `a` and `b`.
/// @param a First buffer.
/// @param b Second buffer.
void swap(Buffer& a, Buffer& b) noexcept;

} // namespace mem
} // namespace iml