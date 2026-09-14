// imagelib/core/memory/Allocator.tpp
//
// ScopedAlloc member definitions. This file is included at the bottom of
// Allocator.h (never include it directly).

#ifndef IMAGELIB_CORE_MEMORY_ALLOCATOR_H_
#error "Include Allocator.h, not Allocator.tpp directly."
#endif

#include "imagelib/core/memory/Memory.h"

namespace iml {
namespace mem {

template <typename T>
inline ScopedAlloc<T>::ScopedAlloc(size_t count, size_t alignment)
    : ptr_(static_cast<T*>(allocate(mulChecked(count, sizeof(T)), alignment))),
      count_(count) {}

template <typename T>
inline ScopedAlloc<T>::~ScopedAlloc() { release(); }

template <typename T>
inline ScopedAlloc<T>::ScopedAlloc(ScopedAlloc&& other) noexcept
    : ptr_(other.ptr_), count_(other.count_) {
    other.ptr_ = nullptr;
    other.count_ = 0;
}

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

template <typename T>
inline void ScopedAlloc<T>::allocateNew(size_t count, size_t alignment) {
    release();
    ptr_ = static_cast<T*>(allocate(mulChecked(count, sizeof(T)), alignment));
    count_ = count;
}

template <typename T>
inline void ScopedAlloc<T>::release() noexcept {
    if (ptr_ != nullptr) {
        deallocate(ptr_);
        ptr_ = nullptr;
        count_ = 0;
    }
}

template <typename T>
inline T* ScopedAlloc<T>::data() noexcept { return ptr_; }

template <typename T>
inline const T* ScopedAlloc<T>::data() const noexcept { return ptr_; }

template <typename T>
inline T& ScopedAlloc<T>::operator[](size_t i) noexcept { return ptr_[i]; }

template <typename T>
inline const T& ScopedAlloc<T>::operator[](size_t i) const noexcept { return ptr_[i]; }

template <typename T>
inline ScopedAlloc<T>::operator bool() const noexcept { return ptr_ != nullptr; }

template <typename T>
inline size_t ScopedAlloc<T>::count() const noexcept { return count_; }

} // namespace mem
} // namespace iml