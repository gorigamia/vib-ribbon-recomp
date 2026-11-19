$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$ninja = (Resolve-Path '.local/build-tools/bin/ninja.exe').Path
cmake -S tools/psxrecomp-src/recompiler -B .local/recompiler-build -G Ninja `
    "-DCMAKE_MAKE_PROGRAM=$ninja" -DCMAKE_C_COMPILER=clang `
    -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release `
    -DPSXRECOMP_ENABLE_CHD=OFF -DBUILD_TESTING=OFF -DFMT_TEST=OFF
if ($LASTEXITCODE -ne 0) { throw 'Recompiler configuration failed' }
cmake --build .local/recompiler-build --target psxrecomp-game psxrecomp-toml psxrecomp-bios --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Recompiler build failed' }

