#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Negative controls for root public headers and facade dependencies.
import pathlib
import shutil
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix="vmm-requirements-") as directory:
    root = pathlib.Path(directory)
    headers = root / "include/vmm"
    headers.mkdir(parents=True)
    shutil.copyfile(HERE / "negative/root_header.hpp", headers / "root_header.hpp")
    def check():
        return subprocess.run([sys.executable, str(HERE / "check_requirements.py"),
                               "--root", str(root)], text=True, capture_output=True)
    result = check()
    assert result.returncode == 1 and "R1 facade/MissingFacadeTest" in result.stdout, result.stdout
    test = root / "tests/facade/MissingFacadeTest/ut_MissingFacadeTest.cpp"
    test.parent.mkdir(parents=True)
    test.write_text("// SPDX-License-Identifier: BSD-3-Clause\n")
    result = check()
    assert result.returncode == 0, result.stdout
    core = headers / "core/bad.hpp"
    core.parent.mkdir()
    core.write_text("// SPDX-License-Identifier: BSD-3-Clause\n#include <vmm/root_header.hpp>\n")
    result = check()
    assert result.returncode == 1 and "R4 core -> facade" in result.stdout, result.stdout
print("requirements negative controls: 3/3 passed")
