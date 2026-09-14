// imagelib/src/memory/Memory.cpp
//
// Out-of-line definitions for the allocator and buffer primitives.

#include "imagelib/core/memory/Allocator.h"
#include "imagelib/core/memory/Buffer.h"
#include "imagelib/core/memory/Memory.h"

#include <cstring>

namespace iml {
namespace mem {

// ---------------------------------------------------------------------------
// Memory primitives
// ---------------------------------------------------------------------------

const void* alignUpPtr(const void* p, size_t alignment) noexcept {
    const uintptr_t v = reinterpret_cast<uintptr_t>(p);
    return reinterpret_cast<const void*>((v + (alignment - 1)) & ~(alignment - 1));
}

void* alignUpPtr(void* p, size_t alignment) noexcept {
    return const_cast<void*>(alignUpPtr(static_cast<const void*>(p), alignment));
}

size_t addChecked(size_t a, size_t b) {
    if (wouldAddOverflow(a, b)) {
        throw IntegerOverflowError("mem::addChecked: size overflow");
    }
    return a + b;
}

size_t mulChecked(size_t a, size_t b) {
    if (wouldMulOverflow(a, b)) {
        throw IntegerOverflowError("mem::mulChecked: size overflow");
    }
    return a * b;
}

size_t mulChecked3(size_t a, size_t b, size_t c) {
    return mulChecked(mulChecked(a, b), c);
}

void copy(void* dst, const void* src, size_t bytes) noexcept {
    if (dst == nullptr || src == nullptr || bytes == 0) return;
    std::memcpy(dst, src, bytes);
}

void move_(void* dst, const void* src, size_t bytes) noexcept {
    if (dst == nullptr || src == nullptr || bytes == 0) return;
    std::memmove(dst, src, bytes);
}

void set(void* dst, uint8 value, size_t bytes) noexcept {
    if (dst == nullptr || bytes == 0) return;
    std::memset(dst, value, bytes);
}

void zero(void* dst, size_t bytes) noexcept {
    if (dst == nullptr || bytes == 0) return;
    std::memset(dst, 0, bytes);
}

int compare(const void* a, const void* b, size_t bytes) noexcept {
    return std::memcmp(a, b, bytes);
}

// ---------------------------------------------------------------------------
// DefaultAllocator
// ---------------------------------------------------------------------------

void* DefaultAllocator::allocate(size_t size, size_t alignment) {
    return allocateDefault(size, alignment);
}

void* DefaultAllocator::allocateDefault(size_t size, size_t alignment) {
    if (!isValidAlignment(alignment)) {
        throw InvalidParameterError("DefaultAllocator: invalid alignment");
    }

    // Layout: [Header][alignment slack][aligned payload .. +size]
    const size_t total = addChecked(addChecked(HeaderSize, size), alignment - 1);
    void* raw = std::malloc(total);
    if (raw == nullptr) {
        throw AllocationError("DefaultAllocator: malloc returned nullptr");
    }

    byte*   aligned = static_cast<byte*>(alignUpPtr(
        static_cast<byte*>(raw) + HeaderSize, alignment));
    Header* h       = reinterpret_cast<Header*>(aligned - HeaderSize);
    h->base         = raw;
    h->size         = size;
    return aligned;
}

void DefaultAllocator::deallocate(void* p) noexcept {
    deallocateDefault(p);
}

void DefaultAllocator::deallocateDefault(void* p) noexcept {
    if (p == nullptr) return;
    Header* h = headerOf(p);
    std::free(h->base);
}

size_t DefaultAllocator::allocationSize(const void* p) const noexcept {
    return allocationSizeDefault(p);
}

size_t DefaultAllocator::allocationSizeDefault(const void* p) noexcept {
    if (p == nullptr) return 0;
    return headerOf(p)->size;
}

// ---------------------------------------------------------------------------
// Buffer
// ---------------------------------------------------------------------------

Buffer::Buffer(size_t size, size_t alignment, IAllocator* alloc)
    : alloc_(alloc ? alloc : &DefaultAllocator::instance()),
      alignment_(alignment) {
    if (!isValidAlignment(alignment_)) {
        throw InvalidParameterError("Buffer: invalid alignment");
    }
    if (size > 0) {
        data_ = static_cast<byte*>(alloc_->allocate(size, alignment_));
        size_ = size;
        capacity_ = size;
    }
}

Buffer::~Buffer() { release(); }

Buffer::Buffer(const Buffer& other)
    : alloc_(other.alloc_), alignment_(other.alignment_) {
    if (other.size_ > 0) {
        data_ = static_cast<byte*>(alloc_->allocate(other.size_, alignment_));
        if (data_ == nullptr) {
            throw AllocationError("Buffer: copy allocation failed");
        }
        copy(data_, other.data_, other.size_);
        size_     = other.size_;
        capacity_ = other.size_;
    }
}

Buffer& Buffer::operator=(const Buffer& other) {
    if (this == &other) return *this;
    // If we can reuse capacity on a compatible allocator, avoid reallocating.
    if (alloc_ == other.alloc_ && alignment_ == other.alignment_
        && capacity_ >= other.size_) {
        copy(data_, other.data_, other.size_);
        size_ = other.size_;
        return *this;
    }
    Buffer tmp(other);
    swap(tmp);
    return *this;
}

Buffer::Buffer(Buffer&& other) noexcept {
    swap(other);
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        release();
        swap(other);
    }
    return *this;
}

void Buffer::reserve(size_t newCapacity) {
    if (newCapacity <= capacity_) return;
    byte* nd = static_cast<byte*>(alloc_->allocate(newCapacity, alignment_));
    if (data_ != nullptr && size_ > 0) {
        copy(nd, data_, size_);
    }
    alloc_->deallocate(data_);
    data_     = nd;
    capacity_ = newCapacity;
}

void Buffer::resize(size_t newSize) {
    if (newSize > capacity_) {
        regrow(newSize);
    } else if (newSize > size_) {
        zero(data_ + size_, newSize - size_);
    }
    size_ = newSize;
}

void Buffer::swap(Buffer& other) noexcept {
    byte*       tmpData       = data_;
    size_t      tmpSize       = size_;
    size_t      tmpCapacity   = capacity_;
    size_t      tmpAlignment  = alignment_;
    IAllocator* tmpAlloc      = alloc_;

    data_      = other.data_;
    size_      = other.size_;
    capacity_  = other.capacity_;
    alignment_ = other.alignment_;
    alloc_     = other.alloc_;

    other.data_      = tmpData;
    other.size_      = tmpSize;
    other.capacity_  = tmpCapacity;
    other.alignment_ = tmpAlignment;
    other.alloc_     = tmpAlloc;
}

void Buffer::release() noexcept {
    if (data_ != nullptr) {
        alloc_->deallocate(data_);
        data_     = nullptr;
        size_     = 0;
        capacity_ = 0;
    }
}

void Buffer::regrow(size_t newCapacity) {
    reserve(newCapacity);
}

} // namespace mem
} // namespace iml