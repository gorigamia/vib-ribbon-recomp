"""Prepare separate module experiments; these are not a combined runtime."""
import json
from pathlib import Path

inventory = json.loads(Path('analysis/executables-europe.json').read_text(encoding='utf-8'))
roles = {'MAIN_G.EXE': 'game', 'MAIN_T.EXE': 'title', 'MAIN_K.EXE': 'tutorial'}
Path('config').mkdir(exist_ok=True)
Path('seeds').mkdir(exist_ok=True)
for exe in inventory['executables']:
    if exe['path'] not in roles:
        continue
    role = roles[exe['path']]
    analysis = json.loads((Path('.local/analysis') / exe['path'] / 'analysis.json').read_text())
    seeds = {int(exe['entry_point'], 16)}
    seeds.update(f['addr'] for f in analysis['functions']
                 if f['reachable'] and not f['is_data'] and f['confidence'] in ('verified', 'high'))
    Path(f'seeds/{role}.txt').write_text(
        '# Static candidates from psxrecomp-analyze; runtime coverage is not established.\n' +
        ''.join(f'0x{address:08X}\n' for address in sorted(seeds)), encoding='utf-8')
    config = f'''# Isolated {role} code-generation experiment. Not a standalone game configuration.
[game]
name = "Vib-Ribbon {role}"
id = "SCES-02873"
exe = ".local/executables/{exe['path']}"
load_address = "{exe['load_address']}"
entry_pc = "{exe['entry_point']}"
text_size = "{hex(exe['payload_size'])}"

[recompiler]
seeds = "seeds/{role}.txt"
out_dir = ".local/generated/{role}"
bios_config = "tools/psxrecomp-src/bios/OpenBIOS.toml"
strict = true
discovery = "reachable"
'''
    Path(f'config/{role}.toml').write_text(config, encoding='utf-8')
    print(f'{role}: {len(seeds)} candidate seeds')

