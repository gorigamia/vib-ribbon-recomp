"""Regression: static fragments must reject invalid original delay slots."""
from pathlib import Path
import struct
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import zlib

sys.path.insert(0, str(Path('tools/psxrecomp-src/tools').resolve()))
import compile_overlays as compiler


class StaticFragmentAuditTest(unittest.TestCase):
    def generate(self, words):
        data = struct.pack('<' + 'I' * len(words), *words)
        base = 0x80019000
        identities = [(base, zlib.crc32(data), [(base & 0x1fffffff, len(data))])]

        def emit(command, **kwargs):
            output = Path(command[command.index('--out-dir') + 1])
            (output / 'fragment_full.c').write_text('/* mocked emitter output */')
            (output / 'fragment_full.ranges').write_text('/* mocked manifest */')
            return SimpleNamespace(returncode=0)

        args = SimpleNamespace(recompiler='unused', game_toml=str(Path('game.toml').resolve()),
                               cps=True, project_root=str(Path('.').resolve()))
        with patch.object(compiler.subprocess, 'run', side_effect=emit), \
             patch.object(compiler, 'patch_generated_c_static', return_value=('source', [base])), \
             patch.object(compiler, 'audit_generated_c', return_value={'unknown_bad': [], 'unsupported_todo_addrs': []}), \
             patch.object(compiler, 'parse_overlay_func_ids', return_value=identities), \
             patch.object(compiler, 'parse_cps_continuation_owners', return_value={}), \
             patch.object(compiler, 'namespace_generated_static', return_value=('source', {base: 'native_entry'})):
            return compiler.generate_interior_fragment_static(base, data, base, len(data),
                                                              base & 0x1fffffff, args, guard_bytes=0)

    def test_valid_return_and_delay_slot_remain_available(self):
        part, reason = self.generate([0x03e00008, 0x00000000])  # jr ra; nop
        self.assertIsNotNone(part)
        self.assertEqual(reason, 'ok')

    def test_branch_in_delay_slot_is_rejected_before_publication(self):
        part, reason = self.generate([0x10000000, 0x10000000, 0x00000000])
        self.assertIsNone(part)
        self.assertIn('generated-c-audit:', reason)
        self.assertIn('control transfer in a delay slot', reason)

    def test_unhashed_delay_slot_is_rejected(self):
        part, reason = self.generate([0x03e00008])
        self.assertIsNone(part)
        self.assertIn('not identity-hashed', reason)


if __name__ == '__main__':
    unittest.main()
