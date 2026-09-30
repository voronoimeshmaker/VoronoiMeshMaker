// ============================================================================
// File: error.hpp
// Description: Error value and Result<T> = std::expected<T, Error> (DEC-016).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <expected>
#include <source_location>
#include <string>
#include <utility>
#include <variant>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/error/catalog.hpp>
#include <vmm/error/error_code.hpp>

namespace vmm {

/// Entity an error refers to, if any.
using EntityRef = std::variant<std::monostate, SiteId, CellId, FaceId, RegionId, PatchId>;

class Error {
public:
    Error(ErrorCode code, std::string context = {}, EntityRef entity = {}, Severity severity = Severity::Error,
          std::source_location where = std::source_location::current())
        : code_(code), severity_(severity), context_(std::move(context)), entity_(entity), where_(where) {}

    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] ErrorCategory category() const noexcept { return category_of(code_); }
    [[nodiscard]] Severity severity() const noexcept { return severity_; }
    [[nodiscard]] const std::string& context() const noexcept { return context_; }
    [[nodiscard]] const EntityRef& entity() const noexcept { return entity_; }
    [[nodiscard]] const std::source_location& where() const noexcept { return where_; }

    /// "[VMM-<code>] <catalogue text>: <context> (<entity>)" in the given language.
    [[nodiscard]] std::string message(Language language) const;
    [[nodiscard]] std::string message() const { return message(current_language()); }

private:
    ErrorCode code_;
    Severity severity_;
    std::string context_;
    EntityRef entity_;
    std::source_location where_;
};

template <class T>
using Result = std::expected<T, Error>;

using Status = Result<void>;

/// Shorthand for returning a failure: `return fail(ErrorCode::X, "context");`.
[[nodiscard]] inline std::unexpected<Error> fail(ErrorCode code, std::string context = {}, EntityRef entity = {},
                                                 std::source_location where = std::source_location::current()) {
    return std::unexpected<Error>(Error(code, std::move(context), entity, Severity::Error, where));
}

}  // namespace vmm
