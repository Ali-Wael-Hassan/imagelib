#pragma once
#define IMAGELIB_CORE_MEMORY_ALLOCATOR_H_
// imagelib/core/memory/Allocator.h
//
// Allocation abstraction.
//
// Ownership rules:
//   - memory allocated through an allocator instance must be returned through
//     the same allocator mechanism. Mixing allocation/deallocation mechanisms
//     is undefined; the default allocator detects mismatches where possible.
//
// Performance design:
//   - IAllocator is the customization point for exotic allocators.
//   - DefaultAllocator exposes non-virtual static fast paths so hot
//     allocation sites (row buffers, tile scratch, codec staging) never pay a
//     virtual call.
//
// Layout of the default allocator (header immediately before the aligned
// payload so deallocate() never needs to know the original alignment):
//
//   [ Header ][ alignment slack ][ payload (aligned) ][ slack .. size ]

#include "imagelib/core/Types.h"
#include "imagelib/core/Error.h"
#include "imagelib/core/memory/Memory.h"

#include <cstddef>
#include <cstdlib>
#include <new>

namespace iml {
namespace mem {

// ---------------------------------------------------------------------------
// IAllocator
// ---------------------------------------------------------------------------

class IAllocator {
public:
    virtual ~IAllocator() = default;

    /// Allocates at least `size` bytes, aligned to `alignment`. Throws
    /// AllocationError on failure, InvalidParameterError for bad alignment.
    virtual void* allocate(size_t size, size_t alignment = defaultAlignment) = 0;

    /// Deallocates a pointer previously returned by allocate(). No-op for
    /// nullptr. Must pair with the same allocator used for allocation.
    virtual void deallocate(void* p) noexcept = 0;

    /// Reports the requested payload size for a live pointer (0 for nullptr).
    virtual size_t allocationSize(const void* p) const noexcept = 0;
};

// ---------------------------------------------------------------------------
// DefaultAllocator
//
// malloc()-backed, header-prefixed aligned allocation.
// ---------------------------------------------------------------------------

class DefaultAllocator final : public IAllocator {
public:
    static constexpr size_t HeaderSize = 2 * sizeof(size_t); // base + size

    void* allocate(size_t size, size_t alignment = defaultAlignment) override;
    void  deallocate(void* p) noexcept override;
    size_t allocationSize(const void* p) const noexcept override;

    /// Non-virtual fast paths.
    static void*  allocateDefault(size_t size, size_t alignment = defaultAlignment);
    static void   deallocateDefault(void* p) noexcept;
    static size_t allocationSizeDefault(const void* p) noexcept;

    /// Singleton "default global" allocator.
    static DefaultAllocator& instance();

private:
    struct Header {
        void*  base; // raw block returned by std::malloc
        size_t size; // requested payload size
    };

    static Header* headerOf(void* aligned) noexcept;
    static const Header* headerOf(const void* aligned) noexcept;
};

// ---------------------------------------------------------------------------
// Convenience free functions routing to the default allocator (no virtuals).
// ---------------------------------------------------------------------------

void* allocate(size_t size, size_t alignment = defaultAlignment);

void deallocate(void* p) noexcept;

size_t allocationSize(const void* p) noexcept;

// ---------------------------------------------------------------------------
// ScopedAlloc: plain RAII owner of one allocation (move-only, NOT a smart
// pointer in the std::unique_ptr sense; explicit ownership + release()).
// ---------------------------------------------------------------------------

template <typename T>
class ScopedAlloc {
public:
    ScopedAlloc() noexcept = default;
    explicit ScopedAlloc(size_t count, size_t alignment = defaultAlignment);

    ~ScopedAlloc();

    ScopedAlloc(const ScopedAlloc&) = delete;
    ScopedAlloc& operator=(const ScopedAlloc&) = delete;

    ScopedAlloc(ScopedAlloc&& other) noexcept;
    ScopedAlloc& operator=(ScopedAlloc&& other) noexcept;

    void allocateNew(size_t count, size_t alignment = defaultAlignment);
    void release() noexcept;

    T* data() noexcept;
    const T* data() const noexcept;
    T& operator[](size_t i) noexcept;
    const T& operator[](size_t i) const noexcept;
    explicit operator bool() const noexcept;
    size_t count() const noexcept;

private:
    T*     ptr_   = nullptr;
    size_t count_ = 0;
};

} // namespace mem
} // namespace iml

#include "imagelib/core/memory/Allocator.tpp"