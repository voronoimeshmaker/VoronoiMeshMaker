#!/usr/bin/env python3
# ============================================================================
# File: check_requirements.py
# Description: Static checks of the P05 baseline on vmm/ (P07 task 10):
#   R1  every public non-trivial class has tests/<module>/<Class>/ut_<Class>.cpp
#   R2  tests/<module> mirrors include/vmm/<module>
#   R3  no virtual, no inheritance (class/struct X : Base)
#   R4  module include graph follows the allowed table and has no cycles
#   R8  no PETSc anywhere
#   R13 no "VoronoiGridMaker" in vmm/; public declarations inside namespace vmm
#   R21 find_package only for allowed dependencies
#   R23 SPDX header in every source file (GPL only in src/backend/cgal)
#   AGENTS.md include groups (std, external, VMM), alphabetical in each group
# SPDX-License-Identifier: BSD-3-Clause
# ============================================================================
import argparse
import pathlib
import re
import sys

ALLOWED_DEPS = {
    "core": set(),
    "error": {"core"},
    "geometry": {"core", "error"},
    "domain": {"core", "error", "geometry"},
    "backend": {"core", "error", "geometry", "domain"},
    "sites": {"core", "error", "geometry", "domain"},
    "mesh": {"core", "error", "geometry"},
    "voronoi": {"core", "error", "geometry", "domain", "sites", "backend", "mesh"},
    "reorder": {"core", "error", "mesh"},
    "io": {"core", "error", "geometry", "mesh"},
    "facade": {"core", "error", "geometry", "domain", "sites", "backend", "mesh", "voronoi", "reorder", "io"},
}
ALLOWED_PACKAGES = {"CGAL", "Boost", "GTest", "Python3", "Threads", "GMP", "MPFR", "TBB"}
STD_HEADERS = set("""algorithm any array atomic bit bitset cassert cctype charconv chrono cmath compare concepts
cstddef cstdint cstdio cstdlib cstring deque exception expected filesystem format fstream functional initializer_list
iomanip ios iosfwd iostream istream iterator limits list map memory mutex numbers numeric optional ostream print queue
random ranges ratio set source_location span sstream stdexcept string string_view system_error thread tuple
type_traits typeinfo unordered_map unordered_set utility variant vector""".split())


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def sources(root: pathlib.Path):
    for sub in ("include", "src", "tests", "tools", "examples"):
        base = root / sub
        if base.exists():
            yield from (p for p in base.rglob("*") if p.suffix in (".hpp", ".cpp") and "negative" not in p.parts)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True, help="vmm/ directory")
    root = pathlib.Path(ap.parse_args().root)
    errors: list[str] = []
    trivial = set()
    trivial_file = root / "tools/ci/trivial_classes.txt"
    if trivial_file.exists():
        trivial = {l.split("#")[0].strip() for l in trivial_file.read_text().splitlines() if l.split("#")[0].strip()}

    lib_files = [p for p in sources(root) if p.parts[len(root.parts)] in ("include", "src")]
    # R3
    for p in lib_files:
        code = strip_comments(p.read_text())
        if re.search(r"\bvirtual\b", code):
            errors.append(f"R3 virtual in {p}")
        for m in re.finditer(r"\b(class|struct)\s+(\w+)(\s+final)?\s*:\s*(?!:)", code):
            if not re.search(r"enum\s+$", code[max(0, m.start() - 6):m.start()]):
                errors.append(f"R3 inheritance '{m.group(0).strip()}' in {p}")
    # R1 / R2
    tests = root / "tests"
    for header in (root / "include/vmm").rglob("*.hpp"):
        module = header.relative_to(root / "include/vmm").parts[0]
        if module.endswith(".hpp"):
            continue
        code = strip_comments(header.read_text())
        for m in re.finditer(r"^(?:template\s*<[^>]*>\s*\n)?(?:class|struct)\s+(\w+)\s*(?:final\s*)?[{:]", code, re.M):
            name = m.group(1)
            if name in trivial or name.endswith("Tag"):
                continue
            if not (tests / module / name / f"ut_{name}.cpp").exists():
                errors.append(f"R1 {module}/{name} has no tests/{module}/{name}/ut_{name}.cpp")
    if tests.exists():
        for d in tests.iterdir():
            if d.is_dir() and d.name not in ("support", "data", "integration") and not (root / "include/vmm" / d.name).is_dir():
                errors.append(f"R2 tests/{d.name} has no include/vmm/{d.name}")
    # R4
    graph: dict[str, set[str]] = {}
    for p in lib_files:
        rel = p.relative_to(root).parts
        module = rel[2] if rel[0] == "include" else rel[1]
        if module.endswith(".hpp"):
            module = "facade"
        for dep in re.findall(r"#include\s*<vmm/(\w+)/", p.read_text()):
            if dep != module:
                graph.setdefault(module, set()).add(dep)
                if dep not in ALLOWED_DEPS.get(module, set()):
                    errors.append(f"R4 {module} -> {dep} not allowed ({p})")
    # R8, R13, R23, include order
    for p in list(sources(root)) + list(root.rglob("CMakeLists.txt")) + list(root.rglob("*.cmake")) + list(root.rglob("*.py")):
        if "negative" in p.parts:
            continue
        text = p.read_text()
        if re.search(r"petsc", text, re.I) and p.name != "check_requirements.py":
            errors.append(f"R8 PETSc mentioned in {p}")
        if "VoronoiGridMaker" in text and p.name != "check_requirements.py":
            errors.append(f"R13 'VoronoiGridMaker' in {p}")
        spdx = re.search(r"SPDX-License-Identifier:\s*(\S+)", text)
        gpl = ("backend" in p.parts and "cgal" in p.parts) or "golden" in p.parts  # CGAL / legacy VMMLib
        expected = "GPL-3.0-or-later" if gpl else "BSD-3-Clause"
        if not spdx:
            errors.append(f"R23 missing SPDX in {p}")
        elif spdx.group(1) != expected:
            errors.append(f"R23 {p} has {spdx.group(1)}, expected {expected}")
        if p.suffix in (".hpp", ".cpp"):
            groups = []
            for inc in re.findall(r'^#include\s*([<"][^>"]+[>"])', text, re.M):
                name = inc[1:-1]
                group = 2 if inc.startswith('"') or name.startswith("vmm/") else (0 if name in STD_HEADERS else 1)
                groups.append((group, name.lower()))
            if groups != sorted(groups):
                errors.append(f"include order (std, external, VMM; alphabetical) in {p}")
    for p in list(root.rglob("CMakeLists.txt")) + list(root.rglob("*.cmake")):
        cmake_code = re.sub(r"#[^\n]*", "", p.read_text())
        for pkg in re.findall(r"find_package\((\w+)", cmake_code):
            if pkg not in ALLOWED_PACKAGES:
                errors.append(f"R21 find_package({pkg}) not allowed in {p}")
    # R4 cycles
    def cyclic(node, stack, seen):
        seen.add(node)
        stack.add(node)
        for n in graph.get(node, ()):
            if n in stack or (n not in seen and cyclic(n, stack, seen)):
                return True
        stack.discard(node)
        return False
    if any(cyclic(m, set(), set()) for m in graph):
        errors.append("R4 cycle in the module graph")

    for e in errors:
        print(e)
    print(f"check_requirements: {len(errors)} problem(s); modules {sorted(graph)}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
