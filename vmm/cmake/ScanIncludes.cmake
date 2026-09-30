# SPDX-License-Identifier: BSD-3-Clause
# Lists every header opened while preprocessing each public header
# (compiler -M on each header) and fails if any CGAL or Boost header is among them.
#
# Inputs (-D): COMPILER, STD_FLAG, INCLUDE_DIR, HEADERS (;-separated),
#              EXPECT_LEAK (ON for the negative control),
#              EXTRA_INCLUDE_DIRS (;-separated, optional: where the negative
#              control finds CGAL when it is not in a system directory).

set(extra "")
foreach(dir IN LISTS EXTRA_INCLUDE_DIRS)
    list(APPEND extra "-I${dir}")
endforeach()
set(leaks "")
set(total 0)
foreach(source IN LISTS HEADERS)
    execute_process(
        COMMAND "${COMPILER}" "${STD_FLAG}" "-I${INCLUDE_DIR}" ${extra} -M -x c++ "${source}"
        OUTPUT_VARIABLE deps
        ERROR_VARIABLE errors
        RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "preprocessing failed for ${source}:\n${errors}")
    endif()
    string(REGEX REPLACE "[ \\\\\n]+" ";" deps "${deps}")
    list(FILTER deps INCLUDE REGEX "\\.(h|hpp|hh|ipp)$|/[a-z_]+$")
    list(LENGTH deps count)
    math(EXPR total "${total} + ${count}")
    foreach(dep IN LISTS deps)
        if(dep MATCHES "/(CGAL|boost|gmp|mpfr)[/.]")
            list(APPEND leaks "${source} -> ${dep}")
        endif()
    endforeach()
endforeach()

list(LENGTH HEADERS n_sources)
list(LENGTH leaks n_leaks)
message(STATUS "scanned ${n_sources} header(s), ${total} dependency entries, ${n_leaks} CGAL/Boost/GMP/MPFR entries")
if(EXPECT_LEAK)
    if(n_leaks EQUAL 0)
        message(FATAL_ERROR "negative control not detected")
    endif()
    list(GET leaks 0 first)
    message(STATUS "negative control detected as expected, first entry: ${first}")
elseif(n_leaks GREATER 0)
    list(JOIN leaks "\n" text)
    message(FATAL_ERROR "public headers reach external headers:\n${text}")
endif()
