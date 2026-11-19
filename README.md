# vib-ribbon-recomp

Static recompilation experiment for PC of **Vib-Ribbon** (PlayStation, PAL, `SCES-02873`) using [PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp). The goal is to study translating the game's modules to C, selecting them with AOT overlays, and linking them to a runtime that plays from the player's own disc.

> This repository **does not contain** ROMs, discs, soundtracks, textures, or any other protected Vib-Ribbon content. You must provide a legal dump of your own copy. The hashes in `analysis/` are only identifiers used to validate that disc locally.

## Current status

- The initial isolated-generation phase produced 79, 578, 522 and 924 dispatch entries for `SCES_028.73`, `MAIN_T.EXE`, `MAIN_K.EXE` and `MAIN_G.EXE` — 46 compiled objects, with no linking or execution. Details are in `analysis/recomp-experiment.json`.
- `aot/overlays.json` describes the three overlaid modules, selected by checking resident bytes instead of merging dispatch tables directly.
- `psx_last_run_report.json` holds the latest known local report: build `v0.3.4-alpha-25-gd35f13c9-dirty`, 8,877 frames, 107,016 native dispatches and 13 interpreter fallbacks, with 97 overlays registered/validated. It is telemetry from a session ended by closing the SDL window — **not** proof of completion or parity with the original.
- The locally generated executable is kept out of Git and was published as [release `v0.1.0-experimental`](https://github.com/gorigamia/vib-ribbon-recomp/releases/tag/v0.1.0-experimental). What is versioned here is the code, configuration and documentation.

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
pwsh tools/build-runtime.ps1 -Python <path\to\python.exe>
```

The resulting runtime reads `game.toml` and requires the configured disc. Intermediate outputs stay in `.local/`, ignored by Git.

## Known limitations

- MP3/audio streaming support is not implemented in this experiment.
- Four internal loader seeds have no host function and fall back to the interpreter.
- Compiling objects does not establish semantic equivalence; there is no parity suite against the original game.
- The flow was validated only on Windows with the toolchain described above.

## License and credits

The code and configuration in this repository are MIT — see `LICENSE`. Third-party components and copyright notices are in `THIRD_PARTY_NOTICES.md`. Vib-Ribbon belongs to its respective rights holders; this project is not affiliated with them.

Further reading: `analysis/ESTUDO.md` (initial static study) and `analysis/RECOMPILACAO.md` (first experiment with the framework). The AOT mechanism is documented in [AOT_SHARDING.md](https://github.com/RetroPortingToolKit/psxrecomp/blob/master/docs/AOT_SHARDING.md). The two report filenames are kept in their original form so existing links stay valid.
