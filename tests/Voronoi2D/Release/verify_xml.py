# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent T10 reader: XML parsing, zlib blocks and mesh connectivity."""
import json
import math
import pathlib
import struct
import sys
import xml.etree.ElementTree as ET
import zlib


def read(path, extra_names=()):
    content = path.read_bytes()
    appended = b""
    if b'<AppendedData' in content:
        start = content.index(b'<AppendedData')
        payload = content.index(b'>_', start) + 2
        end = content.rindex(b'</AppendedData>')
        appended = content[payload:end]
        content = content[:start] + content[end + len(b'</AppendedData>'):]
    root = ET.fromstring(content)
    piece = root.find('UnstructuredGrid/Piece')
    arrays = {}
    for element in root.iter('DataArray'):
        assert element.attrib['Name'] not in arrays, 'Duplicate array name'
        dtype = element.attrib['type']
        if element.attrib['format'] == 'ascii':
            convert = float if dtype.startswith('Float') else int
            values = [convert(x) for x in element.text.split()]
        else:
            offset = int(element.attrib['offset'])
            count, size, last = struct.unpack_from('<III', appended, offset)
            sizes = struct.unpack_from('<' + 'I' * count, appended, offset + 12)
            pos = offset + 12 + count * 4
            raw = bytearray()
            for i, compressed in enumerate(sizes):
                block = zlib.decompress(appended[pos:pos + compressed])
                assert len(block) == (last if i == count - 1 and last else size)
                raw.extend(block)
                pos += compressed
            code = {'Float64': 'd', 'Float32': 'f', 'Int32': 'i', 'UInt8': 'B'}[dtype]
            values = list(struct.unpack('<' + code * (len(raw) // struct.calcsize(code)), raw))
        arrays[element.attrib['Name']] = values
    n, points = int(piece.attrib['NumberOfCells']), int(piece.attrib['NumberOfPoints'])
    assert n == 21760
    assert len(arrays['Points']) == 3 * points
    for element in piece.find('CellData'):
        components = int(element.attrib.get('NumberOfComponents', 1))
        assert components > 0
        assert len(arrays[element.attrib['Name']]) == n * components
    assert len(piece.find('CellData')) == 12 + len(extra_names)
    assert set(extra_names).issubset(arrays)
    parent = list(range(points))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    previous = 0
    area_total = 0
    area_mismatches = 0
    max_relative_area_error = 0
    for i, end in enumerate(arrays['offsets']):
        ids = arrays['connectivity'][previous:end]
        previous = end
        assert len(ids) == arrays['n_faces'][i] == 4
        assert arrays['types'][i] == 7 and len(set(ids)) == len(ids)
        assert all(0 <= v < points for v in ids)
        for v in ids:
            parent[find(v)] = find(ids[0])
        xy = [arrays['Points'][3 * v:3 * v + 2] for v in ids]
        ox, oy = xy[0]
        shifted = [(x - ox, y - oy) for x, y in xy]
        area = sum(x * shifted[(j + 1) % len(ids)][1] - y * shifted[(j + 1) % len(ids)][0]
                   for j, (x, y) in enumerate(shifted)) / 2
        relative_error = abs(area - arrays['V_P'][i]) / area
        max_relative_area_error = max(max_relative_area_error, relative_error)
        area_mismatches += relative_error > 1e-12
        assert abs(area - 300 / 21760) < 1e-13
        area_total += area
    assert previous == len(arrays['connectivity'])
    regions = len({find(i) for i in range(points)})
    assert abs(area_total - 300) < 1e-9
    return arrays, {'points': points, 'expected_points': 257 * 86, 'cells': n, 'connected_regions': regions,
                    'area': area_total, 'bytes': path.stat().st_size,
                    'area_mismatches': area_mismatches,
                    'max_relative_area_error': max_relative_area_error}


def geometry_blocks(path):
    content = path.read_bytes()
    if b'<AppendedData' not in content:
        return content[content.index(b'<Points>'):content.index(b'<CellData>')]
    start = content.index(b'<AppendedData')
    root = ET.fromstring(content[:start] + b'</VTKFile>')
    first_cell_array = root.find('UnstructuredGrid/Piece/CellData/DataArray')
    geometry_size = int(first_cell_array.attrib['offset'])
    payload = content.index(b'>_', start) + 2
    return content[payload:payload + geometry_size]


def verify_extras(folder, base):
    names = ('field_ref', 'grad_ref', 'field_num', 'a&b<"c\'>')
    binary, summary = read(folder / 'cartesian_extra_binary.vtu', names)
    ascii_arrays, _ = read(folder / 'cartesian_extra_ascii.vtu', names)
    for name, values in binary.items():
        other = ascii_arrays[name]
        assert len(values) == len(other)
        assert all(a == b or (math.isnan(a) and math.isnan(b)) for a, b in zip(values, other))
    for name in base:
        assert binary[name] == base[name], f'Built-in array changed: {name}'
    for encoding in ('binary', 'ascii'):
        assert geometry_blocks(folder / f'cartesian_{encoding}.vtu') == geometry_blocks(
            folder / f'cartesian_extra_{encoding}.vtu'), 'Geometry blocks changed'
    for i in range(21760):
        x = -15 + ((i % 256) + 0.5) * 30 / 256
        y = -5 + ((i // 256) + 0.5) * 10 / 85
        assert abs(binary['field_ref'][i] - (2 + 3*x - 5*y)) < 1e-10
        assert binary['grad_ref'][3*i:3*i+3] == [3, -5, 0]
        assert math.isnan(binary['field_num'][i])
    assert binary[names[-1]] == binary['field_ref'], 'Escaped name did not round-trip'
    assert summary['connected_regions'] == 1
    assert summary['bytes'] < 1500000
    assert summary['area_mismatches'] == 0
    return {**summary, 'extra_arrays': len(names), 'geometry_blocks_identical': True,
            'ascii_binary_equal': True, 'analytic_patch_verified': True}


if __name__ == '__main__':
    folder = pathlib.Path(sys.argv[1])
    binary, summary = read(folder / 'cartesian_binary.vtu')
    ascii_arrays, _ = read(folder / 'cartesian_ascii.vtu')
    summary['ascii_binary_equal'] = binary == ascii_arrays
    if '--extras' in sys.argv[2:]:
        summary['extra_arrays_test'] = verify_extras(folder, binary)
    print(json.dumps(summary, indent=2))
    assert binary == ascii_arrays, 'ASCII/binary round-trip differs'
    assert summary['bytes'] < 1500000
    assert summary['connected_regions'] == 1
    assert summary['area_mismatches'] == 0
    assert summary['points'] == summary['expected_points']
