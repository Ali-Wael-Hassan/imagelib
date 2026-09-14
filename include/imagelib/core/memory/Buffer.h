#pragma once
#define IMAGELIB_CORE_MEMORY_BUFFER_H_
// imagelib/core/memory/Buffer.h
//
// Owning raw-memory buffer abstraction built on the allocator layer.
//
// Ownership: Buffer owns its memory and frees it in the destructor through
// the allocator it was created with. It is copyable (deep copy) and movable
// (transfer of ownership). No smart pointers are used.
//
// Thread safety: a Buffer instance is not thread safe for concurrent
// mutation; distinct instances may be freely used from distinct threads.

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/core/memory/Memory.h"
#include "imagelib/core/memory/Allocator.h"

#include <cstddef>

namespace iml {
namespace mem {

class Buffer {
public:
    /// Empty buffer marked for the default allocator.
    Buffer() noexcept = default;

    /// Creates a buffer of `size` bytes aligned to `alignment` using `alloc`.
    /// Throws AllocationError / IntegerOverflowError.
    explicit Buffer(size_t size,
                    size_t alignment = defaultAlignment,
                    IAllocator* alloc = &DefaultAllocator::instance());

    ~Buffer();

    // -- copy (deep) / move (transfer) -------------------------------------

    Buffer(const Buffer& other);
    Buffer& operator=(const Buffer& other);
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    // -- capacity management -----------------------------------------------

    /// Ensures capacity >= newCapacity; copies contents on regrow.
    void reserve(size_t newCapacity);
    /// Changes logical size; grows (zero-filled new tail) or truncates.
    void resize(size_t newSize);
    /// Logical size -> 0 (keeps capacity).
    void clear() noexcept;

    // -- access ------------------------------------------------------------

    byte* data() noexcept;
    const byte* data() const noexcept;
    byte* begin() noexcept;
    byte* end() noexcept;
    const byte* begin() const noexcept;
    const byte* end() const noexcept;

    byte& operator[](size_t i) noexcept;
    const byte& operator[](size_t i) const noexcept;

    size_t size() const noexcept;
    bool   empty() const noexcept;
    size_t capacity() const noexcept;
    size_t alignment() const noexcept;

    IAllocator* allocator() const noexcept;

    void swap(Buffer& other) noexcept;

private:
    void release() noexcept;
    void regrow(size_t newCapacity);

    IAllocator* alloc_      = &DefaultAllocator::instance();
    byte*       data_       = nullptr;
    size_t      size_       = 0;
    size_t      capacity_   = 0;
    size_t      alignment_  = defaultAlignment;
};

void swap(Buffer& a, Buffer& b) noexcept;

} // namespace mem
} // namespace iml