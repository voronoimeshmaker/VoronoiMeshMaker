# Dependencies configuration: the CGAL package, with its headers checked
# against the selected installation (vmm/ finds the rest itself).

find_package(CGAL REQUIRED)
include("${CMAKE_CURRENT_LIST_DIR}/VerifyCGALHeaders.cmake")
vmm_verify_cgal_headers()
