"""Validate original resource structures and preview the road's line geometry."""
import collections
import hashlib
import json
from pathlib import Path
import struct
import zlib

ROOT = Path('.local/resources/GAME_02_FILES.PAK')


def parse_fsl(data):
    cursor = 0

    def words(count, signed=False):
        nonlocal cursor
        if count < 0 or cursor + count * 4 > len(data):
            raise ValueError('FSL table exceeds file bounds')
        values = list(struct.unpack_from('<' + ('i' if signed else 'I') * count, data, cursor))
        cursor += count * 4
        return values

    header = words(9)
    track_ids = words(sum(header[6:9]))
    sections = {'fixed': cursor}
    fixed = [words(words(1)[0]) for _ in range(header[4])]
    sections['distribution'] = cursor
    distribution = []
    for _ in range(header[5]):
        # The first word is opaque: the local disc contains values other than
        # 1. Treating it as an array count (as the community notes suggest)
        # overruns the file. Each observed entry occupies 13 words.
        record = words(13, True)
        distribution.append({'opaque_word': record[0], 'values': record[1:]})
    tables = {}
    for name, width, count in zip(('control', 'pattern', 'event'), (5, 4, 6), header[:3]):
        sections[name] = cursor
        tables[name] = [[words(width, True) for _ in range(words(1)[0])] for _ in range(count)]
        for table in tables[name]:
            if any(a[0] > b[0] for a, b in zip(table, table[1:])):
                raise ValueError('Unsorted time segment table')
    sections['track_index'] = cursor
    indices = [words(3) for _ in range(header[3])]
    if cursor != len(data):
        raise ValueError(f'FSL parser stopped at {cursor:#x}, expected {len(data):#x}')
    for ci, pi, ei in indices:
        if ci >= len(tables['control']) or pi >= len(tables['pattern']) or ei >= len(tables['event']):
            raise ValueError('Invalid FSL track index')
    for table in tables['pattern']:
        for _, _, kind, index in table:
            if kind not in (0, 1, 2) or (kind and index != -1 and not 0 <= index < len(fixed if kind == 1 else distribution)):
                raise ValueError('Invalid FSL pattern reference')
    return {'sha256': hashlib.sha256(data).hexdigest(), 'size': len(data), 'header': header,
            'section_offsets': {k: hex(v) for k, v in sections.items()},
            'fixed_patterns': fixed, 'distribution_pattern_count': len(distribution),
            'track_indices': indices, 'tracks': [
                {'index': i, **{name: {'segments': len(table), 'first_time': table[0][0] if table else None,
                                      'last_time': table[-1][0] if table else None}
                                for name, table in ((n, tables[n][indices[i][j]]) for j, n in enumerate(('control', 'pattern', 'event')))}}
                for i in range(len(indices))]}


def parse_tmd(data):
    magic, flags, count = struct.unpack_from('<3I', data)
    if magic != 0x41 or flags != 0 or 12 + count * 28 > len(data):
        raise ValueError('Expected an unrelocated TMD')
    objects = []
    for index in range(count):
        vo, nv, no, nn, po, np, scale = struct.unpack_from('<6Ii', data, 12 + index * 28)
        if 12 + vo + nv * 8 > len(data) or 12 + no + nn * 8 > len(data):
            raise ValueError('TMD vertex or normal data exceeds file')
        vertices = [struct.unpack_from('<3h', data, 12 + vo + j * 8) for j in range(nv)]
        cursor = 12 + po
        modes = collections.Counter()
        lines = []
        for _ in range(np):
            if cursor + 4 > len(data):
                raise ValueError('Missing primitive header')
            olen, ilen, flag, mode = struct.unpack_from('<4B', data, cursor)
            end = cursor + 4 + ilen * 4
            if end > len(data):
                raise ValueError('Primitive exceeds file')
            modes[hex(mode)] += 1
            if mode & 0xfd == 0x40:  # flat line, opaque or semi-transparent
                if ilen != 2:
                    raise ValueError('Unexpected flat-line packet length')
                a, b = struct.unpack_from('<2H', data, cursor + 8)
                if a >= nv or b >= nv:
                    raise ValueError('Line index exceeds object vertex table')
                lines.append((a, b))
            cursor = end
        objects.append({'vertices': vertices, 'lines': lines, 'scale': scale, 'modes': dict(modes)})
    return objects


def road_preview(objects, output):
    width, height = 1200, 480
    pixels = bytearray([12, 15, 21]) * (width * height)
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
           '<rect width="100%" height="100%" fill="#0c0f15"/>']

    def line(x0, y0, x1, y1, color):
        x0, y0, x1, y1 = map(round, (x0, y0, x1, y1))
        dx, dy = abs(x1 - x0), -abs(y1 - y0)
        sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
        err = dx + dy
        while True:
            if 0 <= x0 < width and 0 <= y0 < height:
                off = (y0 * width + x0) * 3
                pixels[off:off + 3] = bytes(color)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 >= dy:
                err += dy
                x0 += sx
            if e2 <= dx:
                err += dx
                y0 += sy

    for i, obj in enumerate(objects):
        col, row = i % 5, i // 5
        # Road profiles lie in the Y/Z plane (X is zero in this asset).
        vx = [v[2] for v in obj['vertices']]
        vy = [v[1] for v in obj['vertices']]
        factor = min(190 / max(1, max(vx) - min(vx)), 150 / max(1, max(vy) - min(vy)))
        cx, cy = (max(vx) + min(vx)) / 2, (max(vy) + min(vy)) / 2
        coords = [(col * 240 + 120 + (z - cx) * factor, row * 240 + 120 + (y - cy) * factor) for x, y, z in obj['vertices']]
        for a, b in obj['lines']:
            x0, y0 = coords[a]
            x1, y1 = coords[b]
            line(x0, y0, x1, y1, (240, 244, 255))
            svg.append(f'<path d="M{x0:.2f},{y0:.2f} L{x1:.2f},{y1:.2f}" stroke="#f0f4ff" stroke-width="2" fill="none"/>')
        svg.append(f'<text x="{col * 240 + 120}" y="{row * 240 + 220}" text-anchor="middle" fill="#9daec8" font-family="sans-serif" font-size="14">Object {i} / {len(obj["lines"])} lines</text>')
    svg.append('</svg>')
    output.with_suffix('.svg').write_text('\n'.join(svg), encoding='utf-8')

    def chunk(kind, content):
        return struct.pack('>I', len(content)) + kind + content + struct.pack('>I', zlib.crc32(kind + content))

    scanlines = b''.join(b'\0' + pixels[y * width * 3:(y + 1) * width * 3] for y in range(height))
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>2I5B', width, height, 8, 2, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(scanlines)) + chunk(b'IEND', b'')
    output.with_suffix('.png').write_bytes(png)


if __name__ == '__main__':
    report = {'fsl': parse_fsl((ROOT / 'SCRIPT/SYSTEM3.FSL').read_bytes()), 'models': []}
    for file in sorted(ROOT.rglob('*.TMD')):
        objects = parse_tmd(file.read_bytes())
        modes = collections.Counter()
        for obj in objects:
            modes.update(obj['modes'])
        report['models'].append({'path': file.relative_to(ROOT).as_posix(), 'objects': len(objects),
            'vertices': sum(len(o['vertices']) for o in objects), 'primitive_modes': dict(modes),
            'decoded_lines': sum(len(o['lines']) for o in objects)})
        if file.relative_to(ROOT).as_posix() == 'ROAD/ROAD.TMD':
            road_preview(objects, Path('.local/road-geometry'))
    Path('analysis/assets-europe.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'fsl': report['fsl']['section_offsets'], 'tracks': len(report['fsl']['tracks']),
                      'models_validated': len(report['models'])}, indent=2))
