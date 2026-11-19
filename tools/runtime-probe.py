"""Read existing runtime counters over its loopback-only debug interface."""
import argparse
import json
from pathlib import Path
import socket
import time


def rpc(port, command, **parameters):
    with socket.create_connection(('127.0.0.1', port), timeout=10) as client:
        client.sendall((json.dumps({'id': 1, 'cmd': command, **parameters}) + '\n').encode())
        response = bytearray()
        while len(response) < 16 * 1024 * 1024:
            chunk = client.recv(65536)
            if not chunk:
                break
            response.extend(chunk)
            try:
                result = json.loads(response)
            except (ValueError, UnicodeDecodeError):
                continue
            if result.get('id') != 1:
                raise ValueError('Unexpected debug response ID')
            return result
        raise ValueError('Incomplete or oversized debug response')


def snapshot(port):
    commands = ('frame', 'get_registers', 'dispatch_stats', 'dirty_ram_stats',
                'overlay_loader_status', 'audio_stats', 'cdrom_state', 'pad_status', 'gpu_state')
    result = {'host_monotonic': time.monotonic(), 'commands': {}}
    for command in commands:
        result['commands'][command] = rpc(port, command)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=4480)
    parser.add_argument('--samples', type=int, default=3)
    parser.add_argument('--interval', type=float, default=3)
    parser.add_argument('--output', type=Path, default=Path('.local/runtime-probe.json'))
    parser.add_argument('--command', help='Send one diagnostic command instead of collecting samples')
    parser.add_argument('--params', default='{}', help='JSON parameters for --command')
    args = parser.parse_args()
    if args.command:
        result = rpc(args.port, args.command, **json.loads(args.params))
        print(json.dumps(result))
    else:
        if not 1 <= args.samples <= 20 or not 0 <= args.interval <= 30:
            parser.error('Use 1..20 samples and an interval of 0..30 seconds')
        result = {'schema': 'vib-ribbon runtime observations v1',
                  'note': 'Sequential observations, not an atomic snapshot; no oracle comparison.', 'samples': []}
        args.output.parent.mkdir(parents=True, exist_ok=True)
        for index in range(args.samples):
            if index:
                time.sleep(args.interval)
            result['samples'].append(snapshot(args.port))
            args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
            commands = result['samples'][-1]['commands']
            print(json.dumps({'sample': index, 'frame': commands['frame'],
                              'dispatch': commands['dispatch_stats'],
                              'overlay_native': commands['overlay_loader_status'].get('dispatch_native'),
                              'overlay_fallback': commands['overlay_loader_status'].get('dispatch_interp_fallback')}), flush=True)
