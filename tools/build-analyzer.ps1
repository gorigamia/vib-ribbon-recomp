$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$root = (Resolve-Path 'tools/psxrecomp-src/recompiler').Path
New-Item -ItemType Directory -Path '.local/bin' -Force | Out-Null
$units = @(
    'ps1_exe_parser.cpp', 'mips_decoder.cpp', 'function_analysis.cpp',
    'analysis_db.cpp', 'bios_call_names.cpp', 'analysis_export.cpp',
    'widescreen_scan.cpp', 'main_analyze.cpp'
) | ForEach-Object { Join-Path $root "src/$_" }
& clang++ -std=c++20 -O2 -DFMT_HEADER_ONLY '-DFMT_CONSTEVAL=' `
    "-I$root/include" "-I$root/src" "-I$root/../runtime/include" `
    "-I$root/lib/fmt/include" "-I$root/lib/toml11" @units `
    -o .local/bin/psxrecomp-analyze.exe
if ($LASTEXITCODE -ne 0) { throw 'Analyzer compilation failed' }

