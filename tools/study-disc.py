"""Read-only study of the supplied MODE2/2352 disc; extracted data stays local."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct


def inspect(track, inventory, output):
    raw = track.read_bytes()
    if len(raw) % 2352:
        raise ValueError('Data track must contain complete 2352-byte sectors')
    if hashlib.sha256(raw).hexdigest() != inventory['dataTrackSha256']:
        raise ValueError('Disc hash does not match inventory')

    def read(extent, size):
        chunks = []
        for offset in range(0, size, 2048):
            sector = (extent + offset // 2048) * 2352
            if sector + 2352 > len(raw) or raw[sector + 15] != 2:
                raise ValueError('Invalid or missing Mode 2 sector')
            chunks.append(raw[sector + 24:sector + 24 + min(2048, size - offset)])
        return b''.join(chunks)

    output.mkdir(parents=True, exist_ok=True)
    report = {'source_sha256': inventory['dataTrackSha256'], 'executables': [], 'packages': []}
    for item in inventory['files']:
        name = item['path'].split(';')[0]
        data = read(item['extent'], item['size'])
        if data.startswith(b'PS-X EXE'):
            u32 = lambda offset: struct.unpack_from('<I', data, offset)[0]
            payload_size = u32(0x1c)
            if 2048 + payload_size > len(data):
                raise ValueError('Truncated executable: ' + name)
            local_file = output / Path(name).name
            local_file.write_bytes(data)
            load = u32(0x18)
            strings = []
            for match in re.finditer(rb'[\x20-\x7e]{5,}', data[2048:2048 + payload_size]):
                strings.append({'address': hex(load + match.start()), 'text': match.group().decode('ascii')})
            report['executables'].append({
                'path': name, 'sha256': hashlib.sha256(data).hexdigest(),
                'file_size': len(data), 'entry_point': hex(u32(0x10)),
                'load_address': hex(load), 'payload_size': payload_size,
                'load_end_exclusive': hex(load + payload_size),
                'global_pointer': hex(u32(0x14)), 'stack_base': hex(u32(0x30)),
                'strings': strings,
            })
        elif name.endswith('.PAK'):
            count = struct.unpack_from('<I', data)[0]
            if not 0 < count < len(data) // 4:
                raise ValueError('Invalid PAK entry count: ' + name)
            offsets = struct.unpack_from(f'<{count}I', data, 4)
            if offsets[0] != 4 + count * 4 or any(a >= b for a, b in zip(offsets, offsets[1:])):
                raise ValueError('Invalid PAK offset table: ' + name)
            entries = []
            for index, start in enumerate(offsets):
                end = offsets[index + 1] if index + 1 < count else len(data)
                if not start + 28 <= end <= len(data):
                    raise ValueError('PAK entry outside archive: ' + name)
                name_end = data.index(b'\0', start, min(end, start + 256))
                entry_name = data[start:name_end].decode('ascii')
                size_offset = start + ((name_end - start + 4) & ~3)
                payload_offset = size_offset + 4
                size = struct.unpack_from('<I', data, size_offset)[0]
                if size > end - payload_offset or end - payload_offset - size > 3:
                    raise ValueError('PAK payload size mismatch: ' + entry_name)
                # Never allow archive paths to escape the local extraction root.
                relative = Path(entry_name)
                if relative.is_absolute() or '..' in relative.parts or ':' in entry_name or '\\' in entry_name:
                    raise ValueError('Unsafe archive path: ' + entry_name)
                content = data[payload_offset:payload_offset + size]
                target = output.parent / 'resources' / name.replace('/', '_') / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(content)
                entry = {'name': entry_name, 'offset': start, 'size': size,
                         'sha256': hashlib.sha256(content).hexdigest()}
                if entry_name.endswith('.TMD') and size >= 12:
                    magic, flags, objects = struct.unpack_from('<3I', content)
                    if magic == 0x41 and 12 + objects * 28 <= size:
                        entry['tmd'] = {'flags': flags, 'objects': objects,
                            'vertices': sum(struct.unpack_from('<I', content, 12 + j * 28 + 4)[0] for j in range(objects)),
                            'primitives': sum(struct.unpack_from('<I', content, 12 + j * 28 + 20)[0] for j in range(objects))}
                entries.append(entry)
            report['packages'].append({'path': name, 'size': len(data), 'entry_count': count, 'entries': entries,
                'sha256': hashlib.sha256(data).hexdigest(),
                'first_32_words_le': list(struct.unpack_from('<32I', data)),
                'embedded_psx_exe_offsets': [m.start() for m in re.finditer(b'PS-X EXE', data)],
                'embedded_tim_candidates': [m.start() for m in re.finditer(b'\x10\x00\x00\x00', data)][:32]})
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('track', type=Path)
    parser.add_argument('--inventory', type=Path, default=Path('analysis/disc-europe.json'))
    parser.add_argument('--local-output', type=Path, default=Path('.local/executables'))
    parser.add_argument('--report', type=Path, default=Path('analysis/executables-europe.json'))
    args = parser.parse_args()
    inventory = json.loads(args.inventory.read_text(encoding='utf-8-sig'))
    report = inspect(args.track, inventory, args.local_output)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    for exe in report['executables']:
        print(f"{exe['path']}: {exe['load_address']}..{exe['load_end_exclusive']} entry={exe['entry_point']}, {len(exe['strings'])} strings")
