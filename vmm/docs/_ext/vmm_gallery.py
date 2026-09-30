# SPDX-License-Identifier: BSD-3-Clause
"""Gallery of the C++ examples (P13, R29, R30).

For every vmm/examples/<name>/ex_<name>.cpp the extension reads the header
(Title/Description), runs the compiled example (VMM_EXAMPLES_BIN), renders
every .vtu it writes with the "Estuário" figure palette (3D meshes as a
cutaway below the mid-height plane) and generates a page
with text, figure, output, source and download. A failing example stops the
documentation build.
"""
import re
import shutil
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path

# Figure palette (requirements §6.4): media colours and interface colour.
MEDIUM_COLOURS = {"water": "#0072B2", "solid": "#A0703A", "rock": "#A0703A", "soil": "#A0703A", "air": "#D6E4EC", "gas": "#D6E4EC"}
REGION_FALLBACK = ["#0072B2", "#A0703A", "#009E73", "#E69F00", "#CC79A7", "#56B4E9"]


def _header(source):
    text = Path(source).read_text()
    title = re.search(r"^// Title:\s*(.+)$", text, re.M)
    desc = re.findall(r"^// (?:Description:)?\s{0,14}(.+)$", text.split("// SPDX")[0], re.M)
    description = []
    capture = False
    for line in text.splitlines():
        if line.startswith("// Description:"):
            capture = True
            description.append(line.split(":", 1)[1].strip())
        elif capture and line.startswith("//  ") and not line.startswith("// SPDX"):
            description.append(line[2:].strip())
        elif capture:
            break
    return (title.group(1).strip() if title else Path(source).stem), " ".join(description)


def _read_vtu(path):
    root = ET.parse(path).getroot()
    arrays = {a.get("Name"): a.text.split() for a in root.iter("DataArray") if a.get("Name")}
    points_node = next(root.iter("Points")).find("DataArray")
    xyz = [float(v) for v in points_node.text.split()]
    points = [(xyz[i], xyz[i + 1]) for i in range(0, len(xyz), 3)]
    connectivity = [int(v) for v in arrays["connectivity"]]
    offsets = [int(v) for v in arrays["offsets"]]
    cells, start = [], 0
    for end in offsets:
        cells.append(connectivity[start:end])
        start = end
    region = [int(v) for v in arrays.get("region", [])]
    return points, cells, region


def _media_of_regions(vmesh):
    """Medium name of every region, read from the native file next to the .vtu."""
    lines = [l.split() for l in Path(vmesh).read_text().splitlines() if l and not l.startswith("#")]
    media, regions, k = [], [], 0
    while k < len(lines):
        if lines[k][0] == "media":
            media = [lines[k + 1 + j][0] for j in range(int(lines[k][1]))]
        if lines[k][0] == "regions":
            regions = [media[int(lines[k + 1 + j][1])] for j in range(int(lines[k][1]))]
            break
        k += 1
    return regions


def _shade(hex_colour, factor):
    c = [int(hex_colour[i:i + 2], 16) for i in (1, 3, 5)]
    c = [max(0, min(255, int(v * factor))) for v in c]
    return "#" + "".join(f"{v:02X}" for v in c)


def _read_polyhedra(vtu):
    """Points (x, y, z), faces of every VTK_POLYHEDRON cell and the region of each cell."""
    root = ET.parse(vtu).getroot()
    arrays = {a.get("Name"): a.text.split() for a in root.iter("DataArray") if a.get("Name")}
    xyz = [float(v) for v in next(root.iter("Points")).find("DataArray").text.split()]
    points = [tuple(xyz[i:i + 3]) for i in range(0, len(xyz), 3)]
    stream = [int(v) for v in arrays["faces"]]
    cells, k = [], 0
    while k < len(stream):
        nfaces = stream[k]
        k += 1
        faces = []
        for _ in range(nfaces):
            n = stream[k]
            faces.append(stream[k + 1:k + 1 + n])
            k += 1 + n
        cells.append(faces)
    region = [int(v) for v in arrays.get("region", [])]
    return points, cells, region


def _render3d(vtu, png):
    """Cutaway of a 3D mesh: the cells on one side of a mid plane (horizontal for
    one region, vertical for several), without the air, drawn by the outer faces
    of that set, shaded by their orientation."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection

    points, cells, region = _read_polyhedra(vtu)
    vmesh = Path(vtu).with_suffix(".vmesh")
    region_medium = _media_of_regions(vmesh) if vmesh.exists() else []
    xs, ys, zs = [p[0] for p in points], [p[1] for p in points], [p[2] for p in points]
    # One region: cut at mid-height; several regions: a vertical cut at mid-width (y),
    # through the interfaces.
    axis = 1 if len(set(region)) > 1 else 2
    values = (xs, ys, zs)[axis]
    cut = 0.5 * (min(values) + max(values))

    def centroid(faces):
        ids = {i for f in faces for i in f}
        return tuple(sum(points[i][a] for i in ids) / len(ids) for a in range(3))

    # Air hides what lies under it: its cells are left out when there are other media.
    hidden = {r for r, m in enumerate(region_medium) if m in ("air", "gas")}
    if len(hidden) == len(region_medium):
        hidden = set()
    kept = [c for c, faces in enumerate(cells)
            if centroid(faces)[axis] <= cut and not (region and region[c] in hidden)]
    count = {}
    for c in kept:
        for f in cells[c]:
            key = tuple(sorted(f))
            count[key] = count.get(key, 0) + 1
    light = (0.4, -0.5, 0.77)
    polys, colours = [], []
    for c in kept:
        base = REGION_FALLBACK[0]
        if region and region[c] < len(region_medium):
            base = MEDIUM_COLOURS.get(region_medium[region[c]], REGION_FALLBACK[region[c] % 6])
        for f in cells[c]:
            if count[tuple(sorted(f))] > 1:
                continue
            pts = [points[i] for i in f]
            # Newell normal for the shading.
            n = [0.0, 0.0, 0.0]
            for a, b in zip(pts, pts[1:] + pts[:1]):
                n[0] += (a[1] - b[1]) * (a[2] + b[2])
                n[1] += (a[2] - b[2]) * (a[0] + b[0])
                n[2] += (a[0] - b[0]) * (a[1] + b[1])
            length = max(1e-300, sum(x * x for x in n) ** 0.5)
            shade = 0.55 + 0.45 * max(0.0, sum(x / length * l for x, l in zip(n, light)))
            polys.append(pts)
            colours.append(_shade(base, shade))
    fig = plt.figure(figsize=(8, 6), dpi=150)
    ax = fig.add_subplot(projection="3d")
    ax.add_collection3d(Poly3DCollection(polys, facecolors=colours, edgecolors="#1E2528", linewidths=0.15))
    ax.set_xlim(min(xs), max(xs))
    ax.set_ylim(min(ys), max(ys))
    ax.set_zlim(min(zs), max(zs))
    dx, dy, dz = max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs)
    exaggeration = max(1.0, 0.25 * max(dx, dy) / max(dz, 1e-300))  # flat domains: vertical exaggeration
    ax.set_box_aspect((dx, dy, dz * exaggeration))
    ax.view_init(elev=35, azim=-60)
    ax.set_axis_off()
    fig.tight_layout()
    fig.savefig(png, facecolor="#FBFAF7")
    plt.close(fig)


def _render(vtu, png):
    if "Name=\"faceoffsets\"" in Path(vtu).read_text():
        return _render3d(vtu, png)
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.collections import LineCollection, PolyCollection

    points, cells, region = _read_vtu(vtu)
    vmesh = Path(vtu).with_suffix(".vmesh")
    region_medium = _media_of_regions(vmesh) if vmesh.exists() else []
    # Colour by medium (requirements §6.4); regions of the same medium get different shades.
    seen = {}
    region_colour = []
    for r, medium in enumerate(region_medium):
        base = MEDIUM_COLOURS.get(medium, REGION_FALLBACK[r % len(REGION_FALLBACK)])
        n = seen.get(medium, 0)
        seen[medium] = n + 1
        region_colour.append(_shade(base, (1.0, 0.8, 1.2, 0.65)[n % 4]))
    colours = [region_colour[r] if r < len(region_colour) else REGION_FALLBACK[r % 6] for r in region]
    alphas = [0.4 if (r < len(region_medium) and region_medium[r] == "air") else 0.95 for r in region]
    # Interface edges: edges shared by cells of different regions.
    owner = {}
    interfaces = []
    for c, cell in enumerate(cells):
        for k in range(len(cell)):
            a, b = cell[k], cell[(k + 1) % len(cell)]
            key = (min(a, b), max(a, b))
            if key in owner and region and region[owner[key]] != region[c]:
                interfaces.append([points[a], points[b]])
            owner[key] = c
    polys = [[points[i] for i in cell] for cell in cells]
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    aspect = (max(ys) - min(ys)) / max(1e-300, (max(xs) - min(xs)))
    fig, ax = plt.subplots(figsize=(8, max(2.0, min(8.0, 8 * aspect))), dpi=150)
    pc = PolyCollection(polys, facecolors=colours, edgecolors="#1E2528", linewidths=0.15)
    pc.set_alpha(None)
    pc.set_facecolors([matplotlib.colors.to_rgba(c, a) for c, a in zip(colours, alphas)])
    ax.add_collection(pc)
    ax.add_collection(LineCollection(interfaces, colors="#D55E00", linewidths=0.8))
    ax.set_xlim(min(xs), max(xs))
    ax.set_ylim(min(ys), max(ys))
    ax.set_aspect("equal")
    ax.set_axis_off()
    fig.tight_layout()
    fig.savefig(png, facecolor="#FBFAF7")
    plt.close(fig)


def _build_gallery(app):
    source_dir = Path(app.config.vmm_examples_source)
    bin_dir = Path(app.config.vmm_examples_bin) if app.config.vmm_examples_bin else None
    out = Path(app.srcdir) / "gallery"
    shutil.rmtree(out, ignore_errors=True)
    out.mkdir(parents=True)
    index = ["Galeria", "=======", "", "Exemplos compilados e executados no build da documentação.", "",
             ".. toctree::", "   :maxdepth: 1", ""]
    for example in sorted(p for p in source_dir.iterdir() if p.is_dir()):
        name = example.name
        source = example / f"ex_{name}.cpp"
        if not source.exists():
            continue
        title, description = _header(source)
        work = out / name
        work.mkdir()
        shutil.copy(source, work / source.name)
        output = "(exemplo não executado: VMM_EXAMPLES_BIN não definido)"
        figures = []
        if bin_dir is not None:
            exe = bin_dir / f"vmm_ex_{name}"
            result = subprocess.run([str(exe)], cwd=work, capture_output=True, text=True, timeout=1800)
            if result.returncode != 0:
                raise RuntimeError(f"example {name} failed:\n{result.stdout}\n{result.stderr}")
            output = result.stdout
            for vtu in sorted(work.glob("*.vtu")):
                png = vtu.with_suffix(".png")
                _render(vtu, png)
                figures.append(png.name)
        page = [title, "=" * len(title), "", description, ""]
        for fig in figures:
            page += [f".. image:: {name}/{fig}", "   :class: vmm-gallery", "   :width: 100%", ""]
        page += ["Saída", "-----", "", ".. code-block:: text", ""]
        page += ["   " + line for line in output.splitlines()] + [""]
        page += ["Código", "------", "", f":download:`Baixar ex_{name}.cpp <{name}/ex_{name}.cpp>`", "",
                 f".. literalinclude:: {name}/ex_{name}.cpp", "   :language: cpp", ""]
        (out / f"{name}.rst").write_text("\n".join(page))
        index.append(f"   {name}")
    (out / "index.rst").write_text("\n".join(index) + "\n")


def setup(app):
    app.add_config_value("vmm_examples_source", "", "env")
    app.add_config_value("vmm_examples_bin", "", "env")
    app.connect("builder-inited", _build_gallery)
    return {"version": "0.2", "parallel_read_safe": True}
