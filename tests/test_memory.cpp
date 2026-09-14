// tests/test_memory.cpp – Memory utilities + Buffer + DefaultAllocator
#include "tests.h"
#include "imagelib/core/memory/Memory.h"
#include "imagelib/core/memory/Allocator.h"
#include "imagelib/core/memory/Buffer.h"
#include <cstring>

using namespace iml;
using namespace iml::mem;

bool test_memory() {
    // alignUp
    TEST_CHECK(alignUp(0, 16) == 0);
    TEST_CHECK(alignUp(1, 16) == 16);
    TEST_CHECK(alignUp(16, 16) == 16);
    TEST_CHECK(alignUp(17, 16) == 32);
    TEST_CHECK(alignUp(13, 4) == 16);

    // checked arithmetic
    TEST_CHECK(addChecked(2, 2) == 4);
    TEST_CHECK(mulChecked(3, 4) == 12);
    TEST_CHECK(mulChecked3(2, 3, 4) == 24);
    bool overflow = false;
    try { mulChecked(size_t(-1), 2); } catch (const IntegerOverflowError&) { overflow = true; }
    TEST_CHECK(overflow);

    // copy/set/zero/compare
    uint8 s1[32], s2[32];
    for (int i = 0; i < 32; ++i) s1[i] = static_cast<uint8>(i);
    copy(s2, s1, 32);
    TEST_CHECK(compare(s2, s1, 32) == 0);
    set(s1, 0xAB, 16);
    TEST_CHECK(s1[0] == 0xAB && s1[15] == 0xAB && s1[16] == 0x10);
    zero(s1, 8);
    TEST_CHECK(s1[0] == 0 && s1[7] == 0 && s1[8] == 0xAB);

    // DefaultAllocator: raw allocate/deallocate
    {
        void* p = DefaultAllocator::allocateDefault(64);
        TEST_CHECK(p != nullptr);
        TEST_CHECK(reinterpret_cast<uintptr_t>(p) % 16 == 0);
        std::memset(p, 0x5A, 64);
        DefaultAllocator::deallocateDefault(p);
    }

    // DefaultAllocator instance API
    {
        IAllocator& a = DefaultAllocator::instance();
        void* p = a.allocate(32, 16);
        TEST_CHECK(p != nullptr);
        TEST_CHECK(reinterpret_cast<uintptr_t>(p) % 16 == 0);
        a.deallocate(p);
    }

    // Buffer lifecycle
    {
        Buffer b(100);
        TEST_CHECK(b.size() == 100);
        TEST_CHECK(b.capacity() >= 100);
        TEST_CHECK(b.data() != nullptr);
        b[99] = 0x7F;
        b[0] = 0x01;
        TEST_CHECK(b[0] == 0x01 && b[99] == 0x7F);
    }
    {
        Buffer b(9, 64);
        TEST_CHECK(b.size() == 9);
        TEST_CHECK(reinterpret_cast<uintptr_t>(b.data()) % 64 == 0);
    }
    {
        Buffer b(8);
        for (size_t i = 0; i < 8; ++i) b[i] = static_cast<byte>(i);
        b.resize(16);
        TEST_CHECK(b.size() == 16);
        TEST_CHECK(b.capacity() >= 16);
        for (size_t i = 0; i < 8; ++i) TEST_CHECK(b[i] == static_cast<byte>(i));
        b.resize(4);
        TEST_CHECK(b.size() == 4);
    }
    {
        // move transfers ownership; copy deep-copies
        Buffer a(16);
        a[0] = 42;
        Buffer copy(a);
        TEST_CHECK(copy.size() == 16 && copy[0] == 42);
        Buffer moved(std::move(a));
        TEST_CHECK(moved.size() == 16 && moved[0] == 42);
        TEST_CHECK(a.size() == 0 || a.data() == nullptr);
    }
    {
        // default buffer is empty
        Buffer e;
        TEST_CHECK(e.size() == 0 && e.data() == nullptr);
    }

    return true;
}