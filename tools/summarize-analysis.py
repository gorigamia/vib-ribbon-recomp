"""Export compact static evidence; addresses are scoped to each loaded module."""
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess


report = {'method': 'static analysis, no game execution', 'dependencies': {}, 'modules': []}
for name in ('psxrecomp-src', 'open-ribbon-src', 'open-ribbon-docs'):
    report['dependencies'][name] = subprocess.check_output(
        ['git', '-C', str(Path('tools') / name), 'rev-parse', 'HEAD'], text=True).strip()

inventory = json.loads(Path('analysis/executables-europe.json').read_text(encoding='utf-8'))
for exe in inventory['executables']:
    name = exe['path']
    data = (Path('.local/executables') / name).read_bytes()
    root = Path('.local/analysis') / name
    analysis = json.loads((root / 'analysis.json').read_text(encoding='utf-8'))
    refs = json.loads((root / 'refs.json').read_text(encoding='utf-8'))['refs']
    module = {k: exe[k] for k in ('path', 'sha256', 'entry_point', 'load_address', 'payload_size')}
    module['sha1'] = hashlib.sha1(data).hexdigest()
    module['analyzer_stats'] = analysis['stats']
    module['gte_function_candidates'] = sum(f['sig']['gte'] for f in analysis['functions'])
    module['anchors'] = []
    for string in exe['strings']:
        if not re.search(r'\.cpp$|BufferTrackEnd\(|SignalGame|main_[tgk]\.exe', string['text']):
            continue
        address = int(string['address'], 16)
        targets = [r for r in refs if r['target'] == address]
        module['anchors'].append({'text': string['text'], 'address': string['address'],
            'function_candidates': sorted({hex(r['func']) for r in targets}),
            'reference_sites': [hex(r['pc']) for r in targets]})
    module['hardware_pointer_literals'] = []
    load = int(exe['load_address'], 16)
    for offset in range(2048, 2048 + exe['payload_size'], 4):
        word = struct.unpack_from('<I', data, offset)[0]
        if 0x1f801000 <= word <= 0x1f801fff:
            address = load + offset - 2048
            module['hardware_pointer_literals'].append({'address': hex(address), 'value': hex(word),
                'referenced_by': sorted({hex(r['func']) for r in refs if r['target'] == address})})
    # A syntactic startup observation, not a general data-flow proof.
    entry = int(exe['entry_point'], 16) - load + 2048
    for offset in range(entry, min(entry + 256, len(data) - 7), 4):
        hi, lo = struct.unpack_from('<2I', data, offset)
        if hi >> 16 == 0x3c1c and lo >> 16 == 0x279c:
            signed_lo = struct.unpack('<h', struct.pack('<H', lo & 0xffff))[0]
            module['startup_gp_pair'] = {'pc': hex(load + offset - 2048),
                'value': hex((((hi & 0xffff) << 16) + signed_lo) & 0xffffffff)}
            break
    report['modules'].append(module)

expected = Path('tools/open-ribbon-src/sces028.sha').read_text().split()[0]
title = next(m for m in report['modules'] if m['path'] == 'MAIN_T.EXE')
report['open_ribbon_title_sha1_match'] = title['sha1'] == expected
Path('analysis/static-europe.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
for module in report['modules']:
    stats = module['analyzer_stats']
    print(module['path'], stats['total_functions'], 'function candidates,', stats['indirect_unresolved'], 'unresolved indirect transfers')
print('Open Ribbon MAIN_T checksum match:', report['open_ribbon_title_sha1_match'])
