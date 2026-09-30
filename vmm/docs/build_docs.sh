#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Builds the documentation of vmm/ (P13). Gate (AGENTS.md, R30): the class and
# integration tests must pass before the gallery runs the examples.
#   vmm/docs/build_docs.sh <cmake build dir> [python venv]
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build="${1:?cmake build directory}"
venv="${2:-}"
[[ -n "$venv" ]] && source "$venv/bin/activate"
ctest --test-dir "$build" -L vmm -j 4 --timeout 1800 --output-on-failure
out="$build/vmm_docs"
mkdir -p "$out/doxygen"
(cd "$here" && sed "s|^INPUT .*|INPUT = $here/../include/vmm|" Doxyfile > "$out/Doxyfile" && \
    echo "OUTPUT_DIRECTORY = $out/doxygen" >> "$out/Doxyfile" && doxygen "$out/Doxyfile")
export VMM_DOXYGEN_XML="$out/doxygen/xml"
export VMM_EXAMPLES_BIN="$build/vmm/examples"
export VMM_EXAMPLES_SOURCE="$here/../examples"
# Sphinx runs on a copy: the gallery pages it generates stay out of the source tree (R24).
rm -rf "$out/src" && cp -r "$here" "$out/src"
python -m sphinx --keep-going -b html "$out/src" "$out/html"
echo "documentation: $out/html/index.html"
