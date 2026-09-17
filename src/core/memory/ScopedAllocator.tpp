#ifndef IMAGELIB_CORE_MEMORY_SCOPEDALLOCATOR_H_
#error "Include ScopedAllocator.h, not ScopedAllocator.tpp directly."
#endif

#include "imagelib/core/memory/Memory.h"

namespace iml {
namespace mem {

/// Allocates storage for `count` elements aligned to `alignment`.
/// @tparam T Element type of the owned allocation.
/// @param count Number of elements to allocate.
/// @param alignment Byte alignment of the allocation.
/// @throws AllocationError On allocation failure.
/// @throws IntegerOverflowError On size computation overflow.
template <typename T>
inline ScopedAlloc<T>::ScopedAlloc(size_t count, size_t alignment)
    : ptr_(static_cast<T*>(allocate(mulChecked(count, sizeof(T)), alignment))), count_(count) {}

/// Releases the owned allocation.
template <typename T> inline ScopedAlloc<T>::~ScopedAlloc() { release(); }

/// Transfers ownership from `other`.
/// @tparam T Element type of the owned allocation.
/// @param other ScopedAlloc to take ownership from (left empty).
template <typename T>
inline ScopedAlloc<T>::ScopedAlloc(ScopedAlloc&& other) noexcept
    : ptr_(other.ptr_), count_(other.count_) {
    other.ptr_ = nullptr;
    other.count_ = 0;
}

/// Move-assigns ownership from `other`.
/// @tparam T Element type of the owned allocation.
/// @param other ScopedAlloc to take ownership from (left empty).
/// @return Reference to this scoped allocator.
template <typename T>
inline ScopedAlloc<T>& ScopedAlloc<T>::operator=(ScopedAlloc&& other) noexcept {
    if (this != &other) {
        release();
        ptr_ = other.ptr_;
        count_ = other.count_;
        other.ptr_ = nullptr;
        other.count_ = 0;
    }
    return *this;
}

/// Replaces the owned allocation with storage for `count` elements.
/// @tparam T Element type of the owned allocation.
/// @param count Number of elements to allocate.
/// @param alignment Byte alignment of the allocation.
/// @throws AllocationError On allocation failure.
/// @throws IntegerOverflowError On size computation overflow.
template <typename T> inline void ScopedAlloc<T>::allocateNew(size_t count, size_t alignment) {
    release();
    ptr_ = static_cast<T*>(allocate(mulChecked(count, sizeof(T)), alignment));
    count_ = count;
}

/// Releases the owned allocation, leaving the object empty.
template <typename T> inline void ScopedAlloc<T>::release() noexcept {
    if (ptr_ != nullptr) {
        deallocate(ptr_);
        ptr_ = nullptr;
        count_ = 0;
    }
}

/// Returns a pointer to the owned storage.
/// @tparam T Element type of the owned allocation.
/// @return Pointer to the first element (nullptr when empty).
template <typename T> inline T* ScopedAlloc<T>::data() noexcept { return ptr_; }

/// Returns a const pointer to the owned storage.
/// @tparam T Element type of the owned allocation.
/// @return Const pointer to the first element (nullptr when empty).
template <typename T> inline const T* ScopedAlloc<T>::data() const noexcept { return ptr_; }

/// Accesses the element at index `i`.
/// @tparam T Element type of the owned allocation.
/// @param i Zero-based element index.
/// @return Reference to the element at `i`.
template <typename T> inline T& ScopedAlloc<T>::operator[](size_t i) noexcept { return ptr_[i]; }

/// Accesses the element at index `i` (const).
/// @tparam T Element type of the owned allocation.
/// @param i Zero-based element index.
/// @return Const reference to the element at `i`.
template <typename T> inline const T& ScopedAlloc<T>::operator[](size_t i) const noexcept {
    return ptr_[i];
}

/// Returns whether storage is allocated.
/// @tparam T Element type of the owned allocation.
/// @return True when this scoped allocator owns an allocation.
template <typename T> inline ScopedAlloc<T>::operator bool() const noexcept {
    return ptr_ != nullptr;
}

/// Returns the number of owned elements.
/// @tparam T Element type of the owned allocation.
/// @return Element count.
template <typename T> inline size_t ScopedAlloc<T>::count() const noexcept { return count_; }

} // namespace mem
} // namespace iml