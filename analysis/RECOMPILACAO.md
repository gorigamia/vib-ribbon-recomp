# First PSXRecomp experiment

Date: November 1, 2026. Framework revision: a7b37e079e8c6e98e04cd1cd42ba559e95911ea7.

This document updates the state after the static study in `ESTUDO.md`.

## Confirmed result

- Built the psxrecomp-game, psxrecomp-toml and psxrecomp-bios tools with Clang 22.1.8.
- The official probe recognized SCES-02873, eight tracks and 83 initial seeds, with no identification warnings.
- Translated the four local executables to C.
- Independently compiled the C files into 46 PC objects, with no compile errors.

| Module | Dispatch entries emitted | Objects compiled |
|---|---:|---:|
| SCES_028.73 | 79 | 3 |
| MAIN_T.EXE | 578 | 12 |
| MAIN_K.EXE | 522 | 11 |
| MAIN_G.EXE | 924 | 20 |

The structured results are in `recomp-experiment.json`. The original BIN/CUE files were only read. Derived C and object files live in `.local`, ignored by Git.

## Observed limits

The first loader sweep mistook a data string for instructions and emitted a reserved-opcode warning. Reachability-based configuration removed that candidate. Four internal loader seeds still have no host function and were left by the tool for interpreter fallback.

The generator reported no warnings while translating the other three modules with this configuration. That does not establish complete coverage. The compiler also recorded warnings in generated C, such as a register compared with itself in the title module; they are preserved in `.local/objects`.

Compiling objects shows that the output is accepted by the local compiler. It does not demonstrate semantic equivalence or a working game. No final executable was linked to the runtime, no scene was run, and MP3 is still not implemented.

## Next integration

The three larger modules overlap from 0x80019000 and produce coincident function names. This experiment's outputs are deliberately isolated. We should not merge their dispatch files directly into a single executable.

The next experiment should use the framework's overlay mechanism with resident-byte checks to select the code matching the loaded module. After that: link the runtime, open the full CUE disc and verify boot → title → menu → first original song.

Mechanism reference: [PSXRecomp AOT sharding](https://github.com/RetroPortingToolKit/psxrecomp/blob/master/docs/AOT_SHARDING.md). Reproducible scripts are documented in the README.

> Historical filename kept for link stability; the document above is in English.
