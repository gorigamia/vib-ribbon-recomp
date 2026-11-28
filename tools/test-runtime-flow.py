"""Exercise the real guest menus through the loopback debug API (own disc required)."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("probe", ROOT / "tools/runtime-probe.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)
PORT = 4483
EXE = Path(os.environ.get("VIB_FLOW_EXE", str(ROOT / ".local/runtime-build/VibRibbonPC.exe")))
OUT = ROOT / ".local/flow-check"
OUT.mkdir(parents=True, exist_ok=True)
def rpc(command, **args):
    result = probe.rpc(PORT, command, **args)
    assert result.get("ok"), result
    return result

def press(buttons, delay=1.5):
    rpc("press", buttons=buttons, frames=5)
    time.sleep(delay)

def shot(name):
    rpc("screenshot_file", path=str(OUT / (name + ".png")))
    state = rpc("read_ram", addr="0x800763C4", len=12)
    print(name, "guest state", state["hex"], flush=True)
    if name in ("04-own-cd-menu", "05-music-request"):
        ram = rpc("read_ram", addr="0x80000000", len=2097152)
        (ROOT / ".local" / ("custom-menu.ram" if name.startswith("04") else "custom-request.ram")).write_bytes(bytes.fromhex(ram["hex"]))
    host = rpc("menu_host")
    host["guest_state"] = int.from_bytes(bytes.fromhex(state["hex"])[:4], "little")
    print(name, json.dumps(host), flush=True)
    return host

settings_path = EXE.parent / "vib_settings.ini"
original_settings = settings_path.read_bytes()
env = os.environ.copy()
env.pop("PSX_HLE_SCHEDULER", None)
with (OUT / "stdout.log").open("w") as out, (OUT / "stderr.log").open("w") as err:
    process = subprocess.Popen([str(EXE),
        *([] if os.environ.get("VIB_FLOW_DEFAULT_LAUNCH") else ["--game", str(ROOT / "game.toml"), "--bios", str(ROOT / "bios/openbios.bin")]),
        "--hidden-window", "--no-launcher", "--debug-port", str(PORT),
        "--memcard-dir", str(OUT / "saves")], cwd=ROOT, env=env, stdout=out, stderr=err)
    try:
        for _ in range(100):
            time.sleep(1)
            assert process.poll() is None, "runtime exited during boot"
            try:
                if os.environ.get("VIB_FLOW_FAST_BOOT"): rpc("turbo", enabled=1)
                if rpc("frame")["frame"] > 3000: break
            except (OSError, ValueError): pass
        rpc("turbo", enabled=0)
        assert shot("01-title")["open"] == 0
        press(49151, 2)
        assert shot("02-main-menu")["open"] == 0
        # Start must be passed through; it must not open host settings.
        rpc("host_button", name="start")
        time.sleep(0.5)
        assert rpc("menu_host")["open"] == 0
        press(49151, 25)
        host = shot("03-mode-menu")
        assert host["open"] == 0 and host["guest_state"] == 1, host
        press(65471)
        press(49151)
        host = shot("04-own-cd-menu")
        assert host["open"] == 0 and host["guest_state"] == 3, host
        press(49151, 3)
        host = shot("05-music-request")
        assert host["open"] == 1 and host["paused"] == 1, host
        rpc("host_key", name="Escape")
        time.sleep(1)
        assert shot("06-cancelled-picker")["open"] == 0
        time.sleep(1)
        assert rpc("menu_host")["open"] == 0, "cancelled picker reopened"
        press(61439)
        assert shot("07-back-to-own-cd")["open"] == 0
        press(49151)
        assert shot("08-second-request")["open"] == 1
        rpc("host_key", name="Escape")
        time.sleep(0.5)
        press(61439)
        if os.environ.get("VIB_FLOW_AUDIO_TEST"):
            settings_path.write_text(original_settings.decode("utf-8") +
                "\nmusic1=" + str(ROOT / ".local/tone-22050-1.mp3") +
                "\nmusic2=" + str(ROOT / ".local/tone-48000-2.mp3") + "\n", encoding="utf-8")
            press(49151, 10)
            host = shot("09-audio-mounted")
            assert host["open"] == 0 and host["guest_state"] == 6, host
            print("tray:", json.dumps(rpc("disc_tray_log")), flush=True)
            time.sleep(5)
            shot("10-audio-ready")
        else:
            press(61439)
            shot("09-after-back")
        print("PASS: boot, GAME transition, Start, own-CD gating, cancellation, re-entry", flush=True)
    finally:
        settings_path.write_bytes(original_settings)
        process.terminate()
        process.wait(timeout=15)
