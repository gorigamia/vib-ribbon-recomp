# vib-ribbon-recomp

Static recompilation experiment for PC of **Vib-Ribbon** (PlayStation, PAL, `SCES-02873`) using [PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp). The goal is to study translating the game's modules to C, selecting them with AOT overlays, and linking them to a runtime that plays from the player's own disc.

> This repository **does not contain** ROMs, discs, soundtracks, textures, or any other protected Vib-Ribbon content. You must provide a legal dump of your own copy. The hashes in `analysis/` are only identifiers used to validate that disc locally.

## Current status

- The initial isolated-generation phase produced 79, 578, 522 and 924 dispatch entries for `SCES_028.73`, `MAIN_T.EXE`, `MAIN_K.EXE` and `MAIN_G.EXE` — 46 compiled objects, with no linking or execution. Details are in `analysis/recomp-experiment.json`.
- `aot/overlays.json` describes the three overlaid modules, selected by checking resident bytes instead of merging dispatch tables directly.
- `psx_last_run_report.json` holds the latest known local report: build `v0.3.4-alpha-25-gd35f13c9-dirty`, 8,877 frames, 107,016 native dispatches and 13 interpreter fallbacks, with 97 overlays registered/validated. It is telemetry from a session ended by closing the SDL window — **not** proof of completion or parity with the original.
- The locally generated executable is kept out of Git. [Release `v0.1.0-experimental`](https://github.com/gorigamia/vib-ribbon-recomp/releases/tag/v0.1.0-experimental) ships a ready-to-test ZIP (`VibRibbon-Recompiled-v0.1.0-experimental.zip`) with the runtime, `game.toml`, `config/`, `seeds/`, `bios/openbios.bin`, `mods/`, `assets/`, `JOGAR.bat` and `LEIA-ME.txt`. Extract it and run `JOGAR.bat`; it passes `--game game.toml --bios bios\openbios.bin`, working around the absolute config path baked into this build. You still need your own legal PAL `SCES-02873` dump in `disc/`. What is versioned here is the code, configuration and documentation.
- Press **F1** or the controller **Guide** button to open settings. **Start** remains a game button. Controller changes apply immediately, and device connection/disconnection remains active while settings are open. Xbox 360-compatible XInput controllers use the SDL gamepad mapping; `gamecontrollerdb.txt` adds mappings for other supported pads.
- The MP3 selector opens only at **Insert a music CD** in the PAL `SCES-02873` own-CD flow, guarded by the loaded game module and its menu state. Editing a playlist through F1 only saves it; it does not replace audio during startup or normal menus. Existing playlists are prepared when that request arrives. MP3 selection uses the Windows file dialog. Decoding runs in the background and converts mono/stereo source audio to 44.1 kHz stereo CD-DA. Escape cancels without immediately reopening the selector; leave and re-enter the CD request to try again.
- The runtime uses the deterministic thread scheduler (`hle_scheduler = true`) for the title-to-game loading transition. The default `game.toml` is resolved beside the executable, so moving the project does not retain its old absolute path.


## Layout

| Path | Contents |
|---|---|
| `game.toml` | Main configuration: serial, boot executable, seeds, runtime and disc digests. |
| `config/`, `seeds/` | Isolated title, tutorial and game configurations and seeds. |
| `aot/overlays.json` | AOT profile of the three overlaid executables. |
| `analysis/` | Disc and executable inventory, plus reverse-engineering reports. |
| `tools/` | Scripts for analysis, code generation, checks and runtime build. |
| `bios/openbios.bin` | Free PCSX-Redux BIOS used by the runtime; license in `bios/OpenBIOS.LICENSE`. |
| `patches/` | Adjustments applied to the local PSXRecomp checkout. |
| `src/vib_settings.cpp` | F1 in-game overlay, custom-CD MP3 picker, generic USB controller selection and MP3-to-CDDA conversion. |
| `vib_settings.ini`, `gamecontrollerdb.txt` | Settings saved by F1 and SDL gamepad mappings shipped beside the executable. |
| `mods/` | Example enhancement manifests. |
| `psx_last_run_report.json`, `VibRibbon_Recompiled.execution.json` | Local execution/telemetry evidence. |

## How to reproduce (Windows)

1. Install CMake 3.24+, Ninja, Clang/Clang++ 22.x (tested with 22.1.8), Python 3.11+, Node.js and Git. The scripts expect Ninja at `.local/build-tools/bin/ninja.exe`.
2. Clone PSXRecomp into `tools/psxrecomp-src` at revision `a7b37e079e8c6e98e04cd1cd42ba559e95911ea7`. The checkout is ignored by Git.
3. Using your legal dump, put the CUE/BIN in `disc/` and extract `SCES_028.73`, `MAIN_T.EXE`, `MAIN_K.EXE` and `MAIN_G.EXE` into `.local/executables/`. Adjust `game.toml` if the CUE name differs.
4. From the repository root, run:

```powershell
pwsh tools/build-analyzer.ps1
pwsh tools/analyze-executables.ps1
py -3 tools/prepare-recomp.py
pwsh tools/build-recompiler.ps1
pwsh tools/generate-recomp.ps1
pwsh tools/check-recomp.ps1
pwsh tools/build-runtime.ps1 -Python <path\to\python.exe> -Publish
```

`-Publish` updates `VibRibbon_Recompiled.exe` in the project root and saves the previous executable under `.local/backups/`. Close the game before publishing. The resulting runtime reads `game.toml` and requires the configured disc. Intermediate outputs stay in `.local/`, ignored by Git.

## Known limitations

- Custom music is decoded from MP3 into CD-DA sectors when the game asks for a music CD; each file is limited to 600 seconds and a playlist to 80 minutes / 98 tracks. Playback has no gapless crossfade.
- Four internal loader seeds have no host function and fall back to the interpreter.
- Compiling objects does not establish semantic equivalence; there is no parity suite against the original game.
- The flow was validated only on Windows with the toolchain described above.

## License and credits

The code and configuration in this repository are MIT — see `LICENSE`. Third-party components and copyright notices are in `THIRD_PARTY_NOTICES.md`. Vib-Ribbon belongs to its respective rights holders; this project is not affiliated with them.

Further reading: `analysis/ESTUDO.md` (initial static study) and `analysis/RECOMPILACAO.md` (first experiment with the framework). The AOT mechanism is documented in [AOT_SHARDING.md](https://github.com/RetroPortingToolKit/psxrecomp/blob/master/docs/AOT_SHARDING.md). The two report filenames are kept in their original form so existing links stay valid.
