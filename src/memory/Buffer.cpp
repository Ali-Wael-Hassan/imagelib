// imagelib/src/memory/Buffer.cpp
//
// Out-of-line definitions of the in-header Buffer accessors moved out of
// Buffer.h. The storage-management members (ctors, reserve, resize, swap,
// release, regrow) live in Memory.cpp.

#include "imagelib/core/memory/Buffer.h"

#include <cstddef>

namespace iml {
namespace mem {

void Buffer::clear() noexcept { size_ = 0; }

byte* Buffer::data() noexcept { return data_; }
const byte* Buffer::data() const noexcept { return data_; }
byte* Buffer::begin() noexcept { return data_; }
byte* Buffer::end() noexcept { return data_ + size_; }
const byte* Buffer::begin() const noexcept { return data_; }
const byte* Buffer::end() const noexcept { return data_ + size_; }

byte& Buffer::operator[](size_t i) noexcept { return data_[i]; }
const byte& Buffer::operator[](size_t i) const noexcept { return data_[i]; }

size_t Buffer::size() const noexcept { return size_; }
bool   Buffer::empty() const noexcept { return size_ == 0; }
size_t Buffer::capacity() const noexcept { return capacity_; }
size_t Buffer::alignment() const noexcept { return alignment_; }

IAllocator* Buffer::allocator() const noexcept { return alloc_; }

void swap(Buffer& a, Buffer& b) noexcept { a.swap(b); }

} // namespace mem
} // namespace iml