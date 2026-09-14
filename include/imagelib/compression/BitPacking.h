#pragma once
// imagelib/compression/BitPacking.h
//
// Low-level bit manipulation for packed pixel formats and bitstream I/O.
//
// Bit order: all streams in this header are MOST-SIGNIFICANT-BIT-first within a
// byte (classic image stream order). Values are written most-significant bit
// of the value first.
//
// Ownership: BitWriter/reader do not own raw byte inputs; BitWriter owns a
// growable output buffer via mem::Buffer (RAII, no smart pointers).

#include "imagelib/core/Types.h"
#include "imagelib/core/memory/Buffer.h"
#include "imagelib/core/Error.h"

#include <cstdint>
#include <cstring>

namespace iml {
namespace comp {

// ---------------------------------------------------------------------------
// Constants / utilities
// ---------------------------------------------------------------------------

/// Byte count needed to store `bitCount` bits (rounds up).
constexpr uint64 bytesForBits(uint64 bitCount) noexcept {
    return (bitCount + 7u) / 8u;
}

/// Smallest number of bits needed to represent `value` (0 -> 0, 1 -> 1,
/// 5 -> 3, 255 -> 8, 256 -> 9).
constexpr uint32 bitWidth(uint64 value) noexcept {
    uint32 w = 0;
    while (value != 0) { value >>= 1u; ++w; }
    return w;
}

/// Reverses the low `n` bits of `value`.
constexpr uint64 reverseBits(uint64 value, uint32 n) noexcept {
    uint64 r = 0;
    for (uint32 i = 0; i < n; ++i) {
        r = (r << 1u) | (value & 1u);
        value >>= 1u;
    }
    return r;
}

/// Mask with the low `n` bits set (n in [0, 64]).
constexpr uint64 bitMask(uint32 n) noexcept {
    if (n == 0) return 0ull;
    if (n == 64) return ~0ull;
    return (1ull << n) - 1ull;
}

// ---------------------------------------------------------------------------
// BitWriter: MSB-first bitstream sink
// ---------------------------------------------------------------------------

class BitWriter {
public:
    BitWriter() noexcept = default;

    /// Reserves room (optional performance hint).
    void reserve(size_t bytes) { buf_.reserve(bytes); }

    /// Appends whole bytes (fast path, byte-aligned append).
    void writeBytes(const void* src, size_t nBytes) {
        alignToByte();
        const size_t base = buf_.size();
        need(nBytes);
        buf_.resize(base + nBytes);
        std::memcpy(buf_.data() + base, src, nBytes);
    }

    /// Appends the low `nBits` of `value`, MSB-first.
    void writeBits(uint64 value, uint32 nBits) {
        if (nBits == 0) return;
        if (nBits > 64) nBits = 64;
        value &= bitMask(nBits);

        acc_ = (acc_ << nBits) | value;
        bitCount_ += nBits;

        while (bitCount_ >= 8u) {
            bitCount_ -= 8u;
            const uint8 b = static_cast<uint8>(acc_ >> bitCount_);
            const size_t n = buf_.size();
            need(n + 1);
            buf_.resize(n + 1);
            buf_.data()[n] = b;
        }
        acc_ &= bitMask(bitCount_);
    }

    /// Pads with zero bits up to the next byte boundary (no-op if aligned).
    void alignToByte() {
        while (bitCount_ != 0 && bitCount_ < 8u) {
            writeBits(0, 8u - bitCount_);
        }
    }

    /// Total bits written so far.
    uint64 bitsWritten() const noexcept { return buf_.size() * 8u + bitCount_; }

    /// Bytes written, rounded up to a whole byte.
    uint64 sizeInBytes() const noexcept {
        return buf_.size() + (bitCount_ ? 1u : 0u);
    }

    const byte* data() const noexcept { return buf_.data(); }

    /// Rounds out any partial byte and returns a snapshot buffer.
    mem::Buffer finish() {
        alignToByte();
        return std::move(buf_);
    }

private:
    void need(size_t extra) { buf_.reserve(buf_.size() + extra); }

    mem::Buffer buf_;
    uint64      acc_     = 0;
    uint32      bitCount_ = 0; // bits buffered in acc_ (0..7)
};

// ---------------------------------------------------------------------------
// BitReader: MSB-first bitstream source over caller-owned memory
// ---------------------------------------------------------------------------

class BitReader {
public:
    BitReader(const byte* data, size_t sizeBytes) noexcept
        : data_(data), size_(sizeBytes) {}

    /// Reads the next `nBits` (n in [1, 64]) as an unsigned value.
    uint64 readBits(uint32 nBits) {
        if (nBits > 64) nBits = 64;
        while (bitCount_ < nBits) {
            if (pos_ >= size_) throw IntegerOverflowError("BitReader: read past end of stream");
            acc_ = (acc_ << 8u) | data_[pos_++];
            bitCount_ += 8u;
        }
        bitCount_ -= nBits;
        return (acc_ >> bitCount_) & bitMask(nBits);
    }

    uint8 readByte() { return static_cast<uint8>(readBits(8)); }

    void readBytes(void* dst, size_t nBytes) {
        if (pos_ + nBytes > size_) throw IntegerOverflowError("BitReader: read past end of stream");
        std::memcpy(dst, data_ + pos_, nBytes);
        pos_ += nBytes;
        bitCount_ = 0;
    }

    /// Discards any partially-buffered bits up to the next byte boundary.
    void alignToByte() { bitCount_ = 0; }

    /// Absolute bit position within the stream.
    uint64 bitPosition() const noexcept { return pos_ * 8u - bitCount_; }

    bool eof() const noexcept { return bitCount_ == 0 && pos_ >= size_; }
    size_t bytesConsumed() const noexcept { return pos_; }

private:
    const byte* data_;
    size_t      size_;
    size_t      pos_      = 0;
    uint64      acc_      = 0;
    uint32      bitCount_ = 0;
};

} // namespace comp
} // namespace iml