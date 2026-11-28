param([Parameter(Mandatory=$true)][string]$Python, [switch]$Publish)
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
& $Python -c 'import sys; assert sys.version_info >= (3, 11), "Python 3.11+ is required"'
if ($LASTEXITCODE -ne 0) { throw 'Python 3.11+ is required' }
New-Item -ItemType Directory -Path '.local/tmp' -Force | Out-Null
$env:TEMP = (Resolve-Path '.local/tmp').Path
$env:TMP = $env:TEMP
$env:PYTHONUTF8 = '1'
& $Python tools/apply-framework-patches.py
if ($LASTEXITCODE -ne 0) { throw 'Framework patch check failed' }
& $Python tools/test-overlay-fragment-audit.py
if ($LASTEXITCODE -ne 0) { throw 'Fragment regression test failed' }
& "$PSScriptRoot/build-recompiler.ps1"
& .local/recompiler-build/psxrecomp-game.exe --config game.toml *> .local/codegen-boot.log
if ($LASTEXITCODE -ne 0) { throw 'Loader generation failed' }
& $Python tools/psxrecomp-src/tools/aot_overlay_pipeline.py static `
    --profile aot/overlays.json --game-toml game.toml `
    --recompiler .local/recompiler-build/psxrecomp-game.exe `
    --work-dir .local/aot-work --gcc clang --workers 2 --cps --reuse *> .local/aot-static.log
if ($LASTEXITCODE -ne 0) { throw 'AOT generation/audit failed; see .local/aot-static.log' }
$ninja = (Resolve-Path '.local/build-tools/bin/ninja.exe').Path
cmake -S . -B .local/runtime-build -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" `
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release `
    -DPSX_RECOMP_UI=OFF -DPSX_NETPLAY=OFF -DPSX_ENABLE_VULKAN=OFF -DBUILD_TESTING=OFF
if ($LASTEXITCODE -ne 0) { throw 'Runtime configuration failed' }
cmake --build .local/runtime-build --target psx-runtime --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Runtime build failed' }
foreach ($name in @('gamecontrollerdb.txt', 'vib_settings.ini')) {
    $target = Join-Path '.local/runtime-build' $name
    if (-not (Test-Path -LiteralPath $target)) { Copy-Item -LiteralPath $name -Destination $target }
}

if ($Publish) {
    $backupDir = Join-Path '.local/backups' ('build-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
    New-Item -ItemType Directory -Path $backupDir -Force | Out-Null
    foreach ($name in @('VibRibbon_Recompiled.exe', 'VibRibbon_Recompiled.execution.json')) {
        if (Test-Path -LiteralPath $name) { Copy-Item -LiteralPath $name -Destination $backupDir }
    }
    Copy-Item -LiteralPath '.local/runtime-build/VibRibbonPC.exe' -Destination 'VibRibbon_Recompiled.exe' -Force
    Copy-Item -LiteralPath '.local/runtime-build/VibRibbonPC.execution.json' -Destination 'VibRibbon_Recompiled.execution.json' -Force
    Write-Host "Executavel atualizado; versao anterior em $backupDir"
}
