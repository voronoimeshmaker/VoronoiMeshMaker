// ============================================================================
// File: contaminated.hpp
// Description: P05a.1 negative control - a public header that leaks CGAL.
//              The firewall checks must reject it; it is never installed.
// ============================================================================

#pragma once

//==============================================================================
//  External libraries
//==============================================================================
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

namespace vmm::p05a {

using LeakedPoint = CGAL::Exact_predicates_inexact_constructions_kernel::Point_2;

}  // namespace vmm::p05a
