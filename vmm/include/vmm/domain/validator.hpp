// ============================================================================
// File: validator.hpp
// Description: Validation of a Partition2D (DEC-018): voids, regions emptied
//              by the precedence order, fragmented regions, slivers and loop
//              consistency.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/core/types.hpp>
#include <vmm/domain/partition.hpp>
#include <vmm/error/error.hpp>

namespace vmm {

struct ValidationOptions {
    /// Local spacing h used for the sliver test; <= 0 means 1e-3 L.
    Real local_spacing = 0;
    /// A component is a sliver when 2 * area / perimeter < sliver_fraction * h.
    Real sliver_fraction = 0.05;
    bool slivers_are_errors = false;
};

class ValidationReport {
public:
    void add(Error e);
    [[nodiscard]] bool ok() const noexcept { return errors_.empty(); }
    [[nodiscard]] const std::vector<Error>& errors() const noexcept { return errors_; }
    [[nodiscard]] const std::vector<Error>& warnings() const noexcept { return warnings_; }

private:
    std::vector<Error> errors_;
    std::vector<Error> warnings_;
};

[[nodiscard]] ValidationReport validate_partition(const Partition2D& partition, const ValidationOptions& options = {});

}  // namespace vmm
