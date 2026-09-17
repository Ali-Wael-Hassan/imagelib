#include "imagelib/core/memory/Buffer.h"

#include <cstddef>

namespace iml {
namespace mem {

/// Constructs a buffer of `size` bytes aligned to `alignment` using `alloc`.
/// @param size Number of bytes to allocate.
/// @param alignment Byte alignment of the allocation.
/// @param alloc Allocator to use (defaults to the default global allocator).
/// @throws InvalidParameterError For an invalid alignment.
/// @throws AllocationError On allocation failure.
Buffer::Buffer(size_t size, size_t alignment, IAllocator* alloc)
    : alloc_(alloc ? alloc : &DefaultAllocator::instance()), alignment_(alignment) {

    if (!isValidAlignment(alignment_)) {
        throw InvalidParameterError("Buffer: invalid alignment");
    }

    if (size > 0) {
        data_ = static_cast<byte*>(alloc_->allocate(size, alignment_));
        size_ = size;
        capacity_ = size;
    }
}

/// Releases the owned memory.
Buffer::~Buffer() { release(); }

/// Creates a deep copy of `other`.
/// @param other Buffer to copy from.
/// @throws AllocationError On allocation failure.
Buffer::Buffer(const Buffer& other) : alloc_(other.alloc_), alignment_(other.alignment_) {

    if (other.size_ > 0) {
        data_ = static_cast<byte*>(alloc_->allocate(other.size_, alignment_));
        if (data_ == nullptr) {
            throw AllocationError("Buffer: copy allocation failed");
        }

        copy(data_, other.data_, other.size_);
        size_ = other.size_;
        capacity_ = other.size_;
    }
}

/// Copy-assigns from `other`.
/// @param other Buffer to copy from.
/// @return Reference to this buffer.
/// @throws AllocationError On allocation failure.
Buffer& Buffer::operator=(const Buffer& other) {
    if (this == &other)
        return *this;

    if (alloc_ == other.alloc_ && alignment_ == other.alignment_ && capacity_ >= other.size_) {
        copy(data_, other.data_, other.size_);
        size_ = other.size_;
        return *this;
    }

    Buffer tmp(other);
    swap(tmp);
    return *this;
}

/// Transfers ownership from `other`.
/// @param other Buffer to take ownership from (left empty).
Buffer::Buffer(Buffer&& other) noexcept { swap(other); }

/// Move-assigns ownership from `other`.
/// @param other Buffer to take ownership from (left empty).
/// @return Reference to this buffer.
Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        release();
        swap(other);
    }
    return *this;
}

/// Ensures capacity >= newCapacity; copies contents on regrow.
/// @param newCapacity Required capacity.
/// @throws AllocationError On allocation failure.
void Buffer::reserve(size_t newCapacity) {
    if (newCapacity <= capacity_)
        return;

    byte* nd = static_cast<byte*>(alloc_->allocate(newCapacity, alignment_));
    if (data_ != nullptr && size_ > 0) {
        copy(nd, data_, size_);
    }

    alloc_->deallocate(data_);
    data_ = nd;
    capacity_ = newCapacity;
}

/// Changes logical size; grows (zero-filled new tail) or truncates.
/// @param newSize New logical size.
/// @throws AllocationError On allocation failure.
void Buffer::resize(size_t newSize) {
    if (newSize > capacity_) {
        regrow(newSize);
    } else if (newSize > size_) {
        zero(data_ + size_, newSize - size_);
    }

    size_ = newSize;
}

/// Exchanges contents with `other`.
/// @param other Buffer to swap with.
void Buffer::swap(Buffer& other) noexcept {
    byte* tmpData = data_;
    size_t tmpSize = size_;
    size_t tmpCapacity = capacity_;
    size_t tmpAlignment = alignment_;
    IAllocator* tmpAlloc = alloc_;

    data_ = other.data_;
    size_ = other.size_;
    capacity_ = other.capacity_;
    alignment_ = other.alignment_;
    alloc_ = other.alloc_;

    other.data_ = tmpData;
    other.size_ = tmpSize;
    other.capacity_ = tmpCapacity;
    other.alignment_ = tmpAlignment;
    other.alloc_ = tmpAlloc;
}

/// Frees the owned memory and resets the buffer to empty.
void Buffer::release() noexcept {
    if (data_ != nullptr) {
        alloc_->deallocate(data_);
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }
}

/// Grows capacity to `newCapacity`.
/// @param newCapacity Target capacity.
void Buffer::regrow(size_t newCapacity) { reserve(newCapacity); }

/// Sets the logical size to zero, keeping capacity.
void Buffer::clear() noexcept { size_ = 0; }

/// Returns a pointer to the first byte.
/// @return Pointer to the owned data.
byte* Buffer::data() noexcept { return data_; }

/// Returns a const pointer to the first byte.
/// @return Const pointer to the owned data.
const byte* Buffer::data() const noexcept { return data_; }

/// Returns a pointer to the first byte.
/// @return Pointer to the first byte.
byte* Buffer::begin() noexcept { return data_; }

/// Returns a pointer one past the last byte.
/// @return Pointer one past the end.
byte* Buffer::end() noexcept { return data_ + size_; }

/// Returns a const pointer to the first byte.
/// @return Const pointer to the first byte.
const byte* Buffer::begin() const noexcept { return data_; }

/// Returns a const pointer one past the last byte.
/// @return Const pointer one past the end.
const byte* Buffer::end() const noexcept { return data_ + size_; }

/// Accesses the byte at index `i`.
/// @param i Zero-based byte index.
/// @return Reference to the byte at `i`.
byte& Buffer::operator[](size_t i) noexcept { return data_[i]; }

/// Accesses the byte at index `i` (const).
/// @param i Zero-based byte index.
/// @return Const reference to the byte at `i`.
const byte& Buffer::operator[](size_t i) const noexcept { return data_[i]; }

/// Returns the logical size in bytes.
/// @return Number of bytes in the buffer.
size_t Buffer::size() const noexcept { return size_; }

/// Returns whether the buffer is empty.
/// @return True when size() == 0.
bool Buffer::empty() const noexcept { return size_ == 0; }

/// Returns the allocated capacity in bytes.
/// @return Allocated capacity.
size_t Buffer::capacity() const noexcept { return capacity_; }

/// Returns the allocation alignment in bytes.
/// @return Alignment of the owned allocation.
size_t Buffer::alignment() const noexcept { return alignment_; }

/// Returns the allocator used for this buffer.
/// @return Pointer to the owning allocator.
IAllocator* Buffer::allocator() const noexcept { return alloc_; }

/// Exchanges contents of `a` and `b`.
/// @param a First buffer.
/// @param b Second buffer.
void swap(Buffer& a, Buffer& b) noexcept { a.swap(b); }

} // namespace mem
} // namespace iml