#pragma once
// imagelib/core/Error.h
//
// Consistent error strategy for ImageLib. All library errors derive from
// iml::Error (a std::runtime_error). Error codes loosely mirror the exception
// types so callers can switch on a stable enum when they prefer codes over
// dynamic typing.

#include <stdexcept>
#include <string>
#include <cstdint>

namespace iml {

/// Stable, exception-type-independent error codes.
enum class ErrorCode : uint16 {
    None = 0,
    InvalidDimension,     // zero / negative / oversize dimensions
    UnsupportedFormat,    // format or data type not implemented
    InvalidFile,          // file missing, unreadable, or corrupt header
    CodecFailure,         // codec backend failed during encode/decode
    AllocationFailure,    // memory allocation failed
    InvalidParameter,     // bad argument (kernel too big, null view, ...)
    IntegerOverflow,      // size computation overflowed the address space
    UnsupportedBackend,   // requested SIMD/parallel backend not compiled in
    ResourceOwnership,    // allocator/ownership pairing violation
};

/// Base class for every ImageLib error.
class Error : public std::runtime_error {
public:
    explicit Error(ErrorCode code, const std::string& message)
        : std::runtime_error(message), code_(code) {}

    ErrorCode code() const noexcept { return code_; }

private:
    ErrorCode code_;
};

#define IML_DEFINE_ERROR(Name, CodeSuffix)                                      \
    class Name##Error : public Error {                                          \
    public:                                                                     \
        explicit Name##Error(const std::string& message)                         \
            : Error(ErrorCode::CodeSuffix, message) {}                          \
    };

IML_DEFINE_ERROR(InvalidDimension, InvalidDimension)
IML_DEFINE_ERROR(UnsupportedFormat, UnsupportedFormat)
IML_DEFINE_ERROR(InvalidFile, InvalidFile)
IML_DEFINE_ERROR(Codec, CodecFailure)
IML_DEFINE_ERROR(Allocation, AllocationFailure)
IML_DEFINE_ERROR(InvalidParameter, InvalidParameter)
IML_DEFINE_ERROR(IntegerOverflow, IntegerOverflow)
IML_DEFINE_ERROR(UnsupportedBackend, UnsupportedBackend)
IML_DEFINE_ERROR(ResourceOwnership, ResourceOwnership)

#undef IML_DEFINE_ERROR

} // namespace iml