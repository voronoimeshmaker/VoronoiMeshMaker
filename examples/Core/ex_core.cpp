//==============================================================================
// Name        : main.cpp
// Author      : João Flávio  Vasconcellos
// Version     : 1.0
// Description : Test program for the PETSc TAO library
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License, as published by the
// Free Software Foundation, version 3 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//==============================================================================

//==============================================================================
//      C++ standard library includes
//==============================================================================
#include <iomanip>              // std::cout
#include <iostream>             // std::precision, std::setw

//==============================================================================
//      VoronoiMeshMaker includes
//==============================================================================
#include <VoronoiMeshMaker/Core/type.h>
#include <VoronoiMeshMaker/Core/constants.h>
#include <VoronoiMeshMaker/IO/IOHelpers.hpp>

using vmm::Real;

int main() {

//==============================================================================
//      Default header/license printing
//==============================================================================

    vmm::io::HeaderPrinter hp_default(
        "ex_core.cpp",
        "João Flávio Vasconcellos",
        "1.0.0",
        "Example of using classes from the Core folder"
    );
    std::cout << hp_default << "\n";

    std::cout << "\n\n" << vmm::io::PrintLine();
    std::cout << vmm::io::CommentPrinter("Printing library constant values");
    std::cout << vmm::io::PrintLine() << "\n\n";

    std::cout << std::setprecision(15) << std::fixed;
    std::cout << "PI       = " << vmm::constants::kPi << "\n";
    std::cout << std::setprecision(4) << std::scientific;
    std::cout << "EPSILON  = " << vmm::constants::kEpsilon << "\n";
    std::cout << "ZERO     = " << vmm::constants::kZeroTol << "\n";

//==============================================================================
//      Using CGAL types
//==============================================================================

    vmm::gtp::Polygon2D tri;
    tri.push_back(vmm::gtp::Point2D(0.0, 0.0));
    tri.push_back(vmm::gtp::Point2D(1.0, 0.0));
    tri.push_back(vmm::gtp::Point2D(0.0, 1.0));

    const Real area = CGAL::to_double(tri.area());
    std::cout << "Triangle area = " << area << " (expect 0.5)\n";

    if (std::abs(area) < vmm::constants::kZeroTol) {
        std::cout << "Area is ~zero by ZERO tolerance\n";
    }

//==============================================================================
//      End of program
//==============================================================================

    std::cout << "\n\n" << vmm::io::PrintLine();
    std::cout << vmm::io::CommentPrinter(
        "Program ex_core.cpp finished successfully"
    );
    std::cout << vmm::io::PrintLine() << "\n\n";
    return 0;
}
