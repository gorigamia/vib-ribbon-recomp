$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
py -3 tools/prepare-recomp.py
if ($LASTEXITCODE -ne 0) { throw 'Module configuration failed' }
foreach ($role in @('boot', 'title', 'tutorial', 'game')) {
    $config = if ($role -eq 'boot') { 'game.toml' } else { "config/$role.toml" }
    & .local/recompiler-build/psxrecomp-game.exe --config $config *> ".local/codegen-$role.log"
    if ($LASTEXITCODE -ne 0) { throw "Code generation failed; see .local/codegen-$role.log" }
    Write-Output "Generated $role; review .local/codegen-$role.log for coverage warnings"
}
