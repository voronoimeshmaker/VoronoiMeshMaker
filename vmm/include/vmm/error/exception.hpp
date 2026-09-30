// ============================================================================
// File: exception.hpp
// Description: The library's exception type. It has no base class (R3,
//              DEC-016). The library throws it only for violated internal
//              invariants; value_or_throw lets an application turn a failed
//              Result into it (DEC-040).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <source_location>
#include <string>
#include <type_traits>
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

/// @brief Returns the value of a successful Result, or throws vmm::Exception with its error (DEC-040).
/// @param result Result of a library call. An lvalue gives a reference to its value; an rvalue gives the
///        value itself (moved out, no copy of a mesh).
/// @return The value held by `result`.
/// @note For code that prefers exceptions to checking every Result. Catch `const vmm::Exception&` (it does not
///       derive from std::exception); `what()` is the English message, `error().message()` the one in the
///       current language.
/// @par Level
/// Beginner
/// @sa Result, Exception
/// @par Location
/// vmm/error/exception.hpp
/// @par Examples
/// ex_value_or_throw.cpp
template <class T>
    requires(!std::is_void_v<T>)
T& value_or_throw(Result<T>& result) {
    if (!result) throw Exception(result.error());
    return *result;
}

template <class T>
    requires(!std::is_void_v<T>)
const T& value_or_throw(const Result<T>& result) {
    if (!result) throw Exception(result.error());
    return *result;
}

template <class T>
    requires(!std::is_void_v<T>)
T value_or_throw(Result<T>&& result) {
    if (!result) throw Exception(std::move(result).error());
    return std::move(*result);
}

/// Throws vmm::Exception if `status` holds an error (DEC-040).
inline void value_or_throw(const Status& status) {
    if (!status) throw Exception(status.error());
}

}  // namespace vmm
