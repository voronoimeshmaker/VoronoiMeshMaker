# SPDX-License-Identifier: BSD-3-Clause
# Sphinx configuration of the new library's documentation (P13).
# Environment:
#   VMM_DOXYGEN_XML   Doxygen XML of vmm/include (build_docs.sh sets it)
#   VMM_EXAMPLES_BIN  directory with the compiled vmm_ex_* examples
#   VMM_EXAMPLES_SOURCE vmm/examples (the docs are built from a copy)
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / "_ext"))

project = "VoronoiMeshMaker"
author = "VoronoiMeshMaker Team"
copyright = "2026, VoronoiMeshMaker Team"
release = "0.1.0"
language = "pt_BR"

extensions = ["breathe", "myst_parser", "sphinx_design", "sphinx.ext.mathjax", "vmm_gallery"]
templates_path = []
exclude_patterns = ["_build", "gallery_src"]

html_theme = "pydata_sphinx_theme"
html_static_path = ["_static"]
html_css_files = ["estuario.css"]
html_title = "VoronoiMeshMaker 0.1"
html_theme_options = {
    "icon_links": [
        {"name": "GitHub", "url": "https://github.com/voronoimeshmaker/voronoimeshmaker", "icon": "fa-brands fa-github"},
    ],
    "navigation_depth": 3,
    "show_toc_level": 2,
}

breathe_projects = {"vmm": os.environ.get("VMM_DOXYGEN_XML", str(Path(__file__).parent / "_build" / "doxygen" / "xml"))}
breathe_default_project = "vmm"
breathe_domain_by_extension = {"hpp": "cpp"}

locale_dirs = ["locale/"]
gettext_compact = False

vmm_examples_source = os.environ.get("VMM_EXAMPLES_SOURCE", str(Path(__file__).parent.parent / "examples"))
vmm_examples_bin = os.environ.get("VMM_EXAMPLES_BIN", "")
