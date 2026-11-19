# Vib-Ribbon PC Recompilation Study

Date: November 1, 2026. European disc SCES_028.73.

> Historical document from the first phase. The current status of the experiment is in the [README](../README.md) and in `RECOMPILACAO.md`. The original filename is kept for link stability.

## Result and limits

The four executables were extracted and analyzed statically. The 13 PAKs were extracted with bounds validation: 1,707 entries, including per-language copies. The resource readers interpret TMD geometry and stage script tables.

We built and ran the PSXRecomp **analyzer**. We did not build the game or its runtime, did not run any scenes, and no playable PC build existed yet. This stage delivers evidence and tools to guide the experiment.

## Game modules

| Executable | Role | RAM load | Entry point | GP set in code |
|---|---|---|---|---|
| SCES_028.73 | Loader | 0x80010000 | 0x80011F2C | 0x800167CC |
| MAIN_T.EXE | Title and language | 0x80019000 | 0x8002BF78 | 0x80047EB8 |
| MAIN_K.EXE | Tutorial | 0x80019000 | 0x800294B4 | 0x80045418 |
| MAIN_G.EXE | Menus and gameplay | 0x80019000 | 0x8005405C | 0x80075E24 |

Addresses were confirmed from headers and local instructions; roles are corroborated by the [Open Ribbon documentation](https://github.com/open-ribbon/documentation/blob/main/docs/decompilation/general-info.md).

The three larger modules share the same RAM base. We need to identify which module is resident in order to choose the correct recompiled function. The header's initial GP is zero, but each module redefines it in code; relying only on the header would break interpretation of global variables.

The SHA-1 of the local MAIN_T is `7eb79e31da2c7456a1ece52835948b75979ce65a`, exactly the one published in `sces028.sha` from [Open Ribbon](https://github.com/open-ribbon/open-ribbon). Public symbols for the title screen can help with a concrete binary match. They must not be applied by address to the other modules.

## Graphics

The English GAME package contains 31 TMD models. Table and primitive bounds and all decoded line indices were checked. This set contains 6,489 mode-0x40 primitives and 19 mode-0x21 primitives. The tool decodes 0x40 line geometry; it does not yet interpret 0x21 primitives.

| Resource | Objects | Vertices | Primitives |
|---|---:|---:|---:|
| CHARA/PEELOO/MODEL.TMD | 16 | 156 | 176 |
| CHARA/FROG/MODEL.TMD | 14 | 148 | 159 |
| ROAD/ROAD.TMD | 10 | 103 | 93 |
| FONT/FE_FONT.TMD | 113 | 690 | 511 |

The `.local/road-geometry.png` preview was drawn from the original vertices and indices of ROAD/ROAD.TMD, which lies in the Y/Z plane. It uses orthographic projection and independent scale per object: it demonstrates obstacle reading, not reproduction of the game's camera, animations or rasterization.

The simple visual helps, but the game uses models and transforms. ANMs animate objects and ANCs animate the camera. For the first boot, the recommendation is to keep GPU/GTE commands in the runtime; a custom renderer can be studied later once a working reference exists. References: [Open Ribbon TMD](https://github.com/open-ribbon/documentation/blob/main/docs/file-types/tmd.md), [PS1 GPU](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/).

## Packages

There are six GAME PAKs with 242 entries each, six KIOSK PAKs with 36 each, and one TITLE PAK with 39. The data is uncompressed at the PAK file level. The structure is a count, an offset table, a zero-terminated name aligned to four bytes, a size and the contents. The extractor validates lengths, offsets and paths before writing to `.local/resources`.

The documented prefixes 01/02/04/08/10/20 correspond to Japanese/English/German/Spanish/French/Italian; they are not six different stages. [PAK documentation](https://github.com/open-ribbon/documentation/blob/main/docs/file-types/pak.md).

## Obstacle generation

The local SCRIPT/SYSTEM3.FSL is 26,920 bytes and has the same SHA-256 in all six GAME packages. The reader walked the file to its last byte, validating indices and segment ordering.

| Structure | Count / start |
|---|---|
| Fixed patterns | 11 / 0x40 |
| Distributions | 63 / 0xEC |
| Control tables | 7 / 0xDB8 |
| Pattern tables | 7 / 0x56CC |
| Event tables | 7 / 0x5EC8 |
| Track indices | 7 / 0x68D4 |

The seventh set contains 774 control segments, 90 pattern segments and 82 event segments. [Open Ribbon's FSL research](https://github.com/open-ribbon/documentation/blob/main/docs/file-types/fsl.md) identifies this set as the basis for custom CDs and describes how starting positions are chosen in it.

This corrects an earlier hypothesis: **it is not demonstrated that the game converts each beat directly into obstacles**. The existence of patterns and scripts is confirmed. We still need to trace how audio influences selection, timing and pauses. Preserving the algorithm requires understanding that combination.

We found a discrepancy in community documentation: the first field of distributions does not work as a simple count. The local bytes include several values other than 1. Our reader keeps that field opaque and uses 52-byte records, reaching all expected offsets and the exact end of the file. The meaning of those bits remains unknown.

Code anchors obtained from string references in MAIN_G:

- AnalysisScript.cpp: candidates 0x80042A54 and 0x80042D48.
- SceneDraw.cpp: candidate 0x8004CA5C.
- BufferTrackEnd(): bad state: candidate 0x8004FD8C.
- CdSys.cpp: references in the 0x80023D3C–0x80024130 region.

The names indicate code neighborhoods; function boundaries are still analyzer hypotheses. It found 1,355 function candidates in MAIN_G and left 278 indirect jumps without proven targets. These are static discovery counts, not a percentage of a completed port.

## Original music and MP3-only input

The CUE contains one data track and seven audio tracks. The complete map, track numbers and pregaps must be preserved. Building only Track 1 would lose information needed for original playback.

The PSXRecomp runtime code inspected already implements CD-DA, track queries and position/peak reports. It is a test base, without proven compatibility with this game. Review notes and local references are in `static-europe.json`.

Proposed, not yet implemented:

1. Keep the original disc during boot and in original mode.
2. When switching to external music, provide a virtual audio disc fed by the selected MP3.
3. Decode to 16-bit stereo PCM at 44.1 kHz. If the code requests sectors, provide 2,352 bytes per sector: 588 stereo frames, 75 sectors per second.
4. Expose playback, analysis, duration and position on the same timeline. Preserve whatever reads the game actually uses, including reports or buffers when needed.
5. Handle MP3 encoder delay/padding; avoid automatic normalization before understanding how amplitude contributes.

We still need to find out whether the algorithm observes disc PCM, controller reports, SPU buffers, metadata or a combination. The file interface will accept MP3 only, as requested.

## Path to proof of concept

Recommendation: try [PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp), using Open Ribbon as a reference for symbols and formats. The framework has a runtime and overlay support; its analyzer already works on this machine. The framework has interpreter fallback, so a generated build alone does not prove that all code was recompiled.

Remaining success criteria:

1. Generate C for the four modules, review transitions and build the application with the runtime and the full CUE.
2. Demonstrate boot → title → menu → tutorial/first original song, initially preserving PAL timing.
3. Compare graphics, controls and audio with a reference run.
4. Trace CD commands, audio buffers and AnalysisScript candidates during known external music.
5. Integrate MP3 and compare the same song via disc and file, controlling track metadata.

The original files were only read. Executables, resources and disassemblies live in `.local`, ignored by Git. The versioned reports contain metadata and investigation results.
