// ============================================================================
// File: exception.hpp
// Description: The library's exception type. It has no base class (R3,
//              DEC-016) and is thrown only for violated internal invariants.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <source_location>
#include <string>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/error.hpp>

namespace vmm {

class Exception {
public:
    explicit Exception(Error error) : error_(std::move(error)), what_(error_.message(Language::English)) {}

    [[nodiscard]] const Error& error() const noexcept { return error_; }
    [[nodiscard]] const char* what() const noexcept { return what_.c_str(); }

private:
    Error error_;
    std::string what_;
};

/// Throws vmm::Exception. Reserved for broken internal invariants (bugs);
/// predictable failures are returned as Result<T>.
[[noreturn]] void raise(ErrorCode code, std::string context = {}, EntityRef entity = {},
                        std::source_location where = std::source_location::current());

}  // namespace vmm
