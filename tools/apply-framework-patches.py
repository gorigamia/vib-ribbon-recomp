"""Apply only the reviewed local patches, refusing mismatched framework sources."""
from pathlib import Path
import subprocess

root = Path('tools/psxrecomp-src').resolve()
for name in ('psxrecomp-overlay-bios-profile.patch', 'psxrecomp-static-fragment-audit.patch',
             'psxrecomp-vib-settings.patch'):
    patch = str((Path('patches') / name).resolve())
    check = subprocess.run(['git', '-C', str(root), 'apply', '--reverse', '--check', patch], capture_output=True)
    if check.returncode == 0:
        print(name + ': already applied')
        continue
    check = subprocess.run(['git', '-C', str(root), 'apply', '--check', patch], capture_output=True)
    if check.returncode:
        raise SystemExit(name + ': framework differs; inspect manually\n' + check.stderr.decode(errors='replace'))
    subprocess.run(['git', '-C', str(root), 'apply', patch], check=True)
    print(name + ': applied')
