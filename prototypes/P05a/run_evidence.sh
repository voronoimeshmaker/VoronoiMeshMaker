#!/usr/bin/env bash
# P05a evidence script (DEC-012): prints the environment, then configures,
# builds, tests and runs prototypes/P05a in a build directory outside the
# source tree. Exits non-zero if any step or invariant fails.
#
#   prototypes/P05a/run_evidence.sh [build_dir]             # Release
#   P05A_SANITIZE=1 prototypes/P05a/run_evidence.sh         # Debug + ASan/UBSan (all checks)
#   P05A_SANITIZE=novptr prototypes/P05a/run_evidence.sh    # same, without -fsanitize=vptr
#
# The full UBSan run stops inside CGAL's Arrangement_2 iterators (downcast
# reported by the vptr check); "novptr" checks everything else.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build="${1:-/tmp/p05a_build}"
config=(-DCMAKE_BUILD_TYPE=Release)
sanitize_flags="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all"
case "${P05A_SANITIZE:-0}" in
    1) build="${build}_sanitize"
       config=(-DCMAKE_BUILD_TYPE=Debug "-DCMAKE_CXX_FLAGS=${sanitize_flags}") ;;
    novptr) build="${build}_sanitize_novptr"
       config=(-DCMAKE_BUILD_TYPE=Debug "-DCMAKE_CXX_FLAGS=${sanitize_flags} -fno-sanitize=vptr") ;;
esac

echo "== environment"
echo "WSL_DISTRO_NAME=${WSL_DISTRO_NAME:-unset}"
grep PRETTY_NAME /etc/os-release
uname -r
c++ --version | head -n 1
cmake --version | head -n 1
echo "ninja $(ninja --version)"
echo "repository $(git -C "$here" rev-parse --short HEAD) ($(git -C "$here" status --short -- . | wc -l) changed/untracked entries under prototypes/P05a)"
echo "build dir ${build} (${config[*]})"

echo "== configure"
cmake -S "$here" -B "$build" -G Ninja "${config[@]}" | grep -E "P05a|CGAL|Boost|Configuring done"
echo "== build"
cmake --build "$build" | tail -n 1
echo "== ctest"
ctest --test-dir "$build" --output-on-failure
for app in p05a_core_only p05a_1_csr p05a_2_multiregion p05a_3_box3d; do
    echo "== ${app}"
    "$build/$app"
done
echo "== all P05a checks passed"
