import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';

const source = process.argv[2];
if (!source) throw new Error('Usage: node tools/inspect-disc.mjs <data-track.bin>');
const disc = fs.readFileSync(source);
const sectorSize = 2352;
function read(extent, size) {
  const output = Buffer.alloc(size);
  for (let offset = 0; offset < size; offset += 2048) {
    const start = (extent + Math.floor(offset / 2048)) * sectorSize + 24;
    disc.copy(output, offset, start, start + Math.min(2048, size - offset));
  }
  return output;
}
const pvd = read(16, 2048);
if (pvd.toString('ascii', 1, 6) !== 'CD001') throw new Error('Expected MODE2/2352 ISO9660 data track');
const files = [];
function walk(extent, size, prefix = '') {
  const data = read(extent, size);
  for (let pos = 0; pos < data.length;) {
    const length = data[pos];
    if (!length) { pos = (Math.floor(pos / 2048) + 1) * 2048; continue; }
    const entry = data.subarray(pos, pos + length);
    const nameLength = entry[32];
    const name = entry.toString('ascii', 33, 33 + nameLength);
    pos += length;
    if (name === '\x00' || name === '\x01') continue;
    const item = {path: prefix + name, extent: entry.readUInt32LE(2), size: entry.readUInt32LE(10)};
    if (entry[25] & 2) walk(item.extent, item.size, item.path + '/');
    else files.push(item);
  }
}
walk(pvd.readUInt32LE(158), pvd.readUInt32LE(166));
const system = files.find(f => /^SYSTEM\.CNF(;1)?$/i.test(f.path));
const config = system ? read(system.extent, system.size).toString('ascii') : null;
const bootName = config?.match(/cdrom:\\([^\s;]+)/i)?.[1];
const executable = files.find(f => f.path.replace(/;\d+$/, '').toLowerCase() === bootName?.toLowerCase());
let boot = null;
if (executable) {
  const bytes = read(executable.extent, executable.size);
  boot = {path: executable.path, signature: bytes.toString('ascii', 0, 8), sha256: crypto.createHash('sha256').update(bytes).digest('hex'), entryPoint: '0x' + bytes.readUInt32LE(16).toString(16), globalPointer: '0x' + bytes.readUInt32LE(20).toString(16), loadAddress: '0x' + bytes.readUInt32LE(24).toString(16), payloadSize: bytes.readUInt32LE(28), stackBase: '0x' + bytes.readUInt32LE(48).toString(16)};
}
const report = {dataTrack: path.basename(source), dataTrackSha256: crypto.createHash('sha256').update(disc).digest('hex'), volume: pvd.toString('ascii', 40, 72).trim(), config, boot, files};
console.log(JSON.stringify(report, null, 2));
