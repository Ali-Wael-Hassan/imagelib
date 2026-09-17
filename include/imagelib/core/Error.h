#pragma once
/// @file
/// Consistent error strategy for ImageLib: all library errors derive from
/// iml::Error (a std::runtime_error).

#include <stdexcept>
#include <string>
#include <cstdint>

namespace iml {

/// Stable, exception-type-independent error codes.
enum class ErrorCode : uint16 {
    /// No error.
    None = 0,
    /// Zero, negative, or oversize dimensions.
    InvalidDimension,
    /// Format or data type not implemented.
    UnsupportedFormat,
    /// File missing, unreadable, or corrupt header.
    InvalidFile,
    /// Codec backend failed during encode/decode.
    CodecFailure,
    /// Memory allocation failed.
    AllocationFailure,
    /// Bad argument (kernel too big, null view, ...).
    InvalidParameter,
    /// Size computation overflowed the address space.
    IntegerOverflow,
    /// Requested SIMD/parallel backend not compiled in.
    UnsupportedBackend,
    /// Allocator/ownership pairing violation.
    ResourceOwnership,
};

/// Base class for every ImageLib error.
class Error : public std::runtime_error {
  public:
    /// Constructs an error with the given code and message.
    /// @param code The stable error code.
    /// @param message Human-readable description of the failure.
    /// @throws std::bad_alloc If the message cannot be stored.
    explicit Error(ErrorCode code, const std::string& message)
        : std::runtime_error(message), code_(code) {}

    /// Returns the stable error code.
    /// @return The ErrorCode passed at construction.
    ErrorCode code() const noexcept { return code_; }

  private:
    /// Stable error code for this error.
    ErrorCode code_;
};

/// Defines a concrete error class deriving from Error with a fixed code.
#define IML_DEFINE_ERROR(Name, CodeSuffix)                                                         \
    class Name##Error : public Error {                                                             \
      public:                                                                                      \
        explicit Name##Error(const std::string& message)                                           \
            : Error(ErrorCode::CodeSuffix, message) {}                                             \
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