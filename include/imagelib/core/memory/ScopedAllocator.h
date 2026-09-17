#pragma once
/// @file
/// ScopedAlloc RAII ownership of a single allocation.
#define IMAGELIB_CORE_MEMORY_SCOPEDALLOCATOR_H_

#include "imagelib/core/memory/DefaultAllocator.h"

#include <cstddef>

namespace iml {
namespace mem {

/// Move-only RAII owner of a single allocation with explicit release().
/// @tparam T Element type of the owned allocation.
template <typename T> class ScopedAlloc {
  public:
    /// Empty scoped allocator owning nothing.
    ScopedAlloc() noexcept = default;

    /// Allocates storage for `count` elements aligned to `alignment`.
    /// @param count Number of elements to allocate.
    /// @param alignment Byte alignment of the allocation.
    /// @throws AllocationError On allocation failure.
    /// @throws IntegerOverflowError On size computation overflow.
    explicit ScopedAlloc(size_t count, size_t alignment = defaultAlignment);

    /// Releases the owned allocation.
    ~ScopedAlloc();

    /// Copy construction is disabled (move-only).
    ScopedAlloc(const ScopedAlloc&) = delete;

    /// Copy assignment is disabled (move-only).
    ScopedAlloc& operator=(const ScopedAlloc&) = delete;

    /// Transfers ownership from `other`.
    /// @param other ScopedAlloc to take ownership from (left empty).
    ScopedAlloc(ScopedAlloc&& other) noexcept;

    /// Move-assigns ownership from `other`.
    /// @param other ScopedAlloc to take ownership from (left empty).
    /// @return Reference to this scoped allocator.
    ScopedAlloc& operator=(ScopedAlloc&& other) noexcept;

    /// Replaces the owned allocation with storage for `count` elements.
    /// @param count Number of elements to allocate.
    /// @param alignment Byte alignment of the allocation.
    /// @throws AllocationError On allocation failure.
    /// @throws IntegerOverflowError On size computation overflow.
    void allocateNew(size_t count, size_t alignment = defaultAlignment);

    /// Releases the owned allocation, leaving the object empty.
    void release() noexcept;

    /// Returns a pointer to the owned storage.
    /// @return Pointer to the first element (nullptr when empty).
    T* data() noexcept;

    /// Returns a const pointer to the owned storage.
    /// @return Const pointer to the first element (nullptr when empty).
    const T* data() const noexcept;

    /// Accesses the element at index `i`.
    /// @param i Zero-based element index.
    /// @return Reference to the element at `i`.
    T& operator[](size_t i) noexcept;

    /// Accesses the element at index `i` (const).
    /// @param i Zero-based element index.
    /// @return Const reference to the element at `i`.
    const T& operator[](size_t i) const noexcept;

    /// Returns whether storage is allocated.
    /// @return True when this scoped allocator owns an allocation.
    explicit operator bool() const noexcept;

    /// Returns the number of owned elements.
    /// @return Element count.
    size_t count() const noexcept;

  private:
    /// Pointer to the owned storage.
    T* ptr_ = nullptr;
    /// Number of owned elements.
    size_t count_ = 0;
};

} // namespace mem
} // namespace iml

#include "core/memory/ScopedAllocator.tpp"