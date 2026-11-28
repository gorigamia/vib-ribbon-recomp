"""Generate original test tones with an installed libmp3lame.dll (no music assets)."""
import argparse
import ctypes
import math
from pathlib import Path
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--encoder", default="C:/Program Files/Audacity/libmp3lame.dll")
args = parser.parse_args()
lame = ctypes.CDLL(args.encoder)
lame.lame_init.restype = ctypes.c_void_p
for name in ("lame_set_in_samplerate", "lame_set_num_channels", "lame_set_brate"):
    getattr(lame, name).argtypes = [ctypes.c_void_p, ctypes.c_int]
lame.lame_init_params.argtypes = [ctypes.c_void_p]
lame.lame_encode_buffer.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_short), ctypes.POINTER(ctypes.c_short), ctypes.c_int, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int]
lame.lame_encode_flush.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int]
lame.lame_close.argtypes = [ctypes.c_void_p]
root = Path(__file__).resolve().parent.parent / ".local"
root.mkdir(exist_ok=True)
for rate, channels in ((22050, 1), (48000, 2)):
    handle = lame.lame_init()
    try:
        lame.lame_set_in_samplerate(handle, rate)
        lame.lame_set_num_channels(handle, channels)
        lame.lame_set_brate(handle, 128)
        assert lame.lame_init_params(handle) >= 0
        n = rate * 8
        samples = (ctypes.c_short * n)(*[int(2000 * math.sin(2 * math.pi * 440 * i / rate)) for i in range(n)])
        out = (ctypes.c_ubyte * (n * 2 + 7200))()
        size = lame.lame_encode_buffer(handle, samples, samples, n, out, len(out))
        assert size > 0
        data = bytes(out[:size])
        size = lame.lame_encode_flush(handle, out, len(out))
        assert size >= 0
        (root / f"tone-{rate}-{channels}.mp3").write_bytes(data + bytes(out[:size]))
    finally:
        lame.lame_close(handle)
