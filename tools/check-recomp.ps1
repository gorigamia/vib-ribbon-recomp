# Compile isolated generated files to x86-64 objects. This does not link or run the game.
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$results = @()
foreach ($role in @('boot', 'title', 'tutorial', 'game')) {
    $target = ".local/objects/$role"
    New-Item -ItemType Directory -Path $target -Force | Out-Null
    $sources = Get-ChildItem ".local/generated/$role" -Filter '*_full_*.c'
    $sources += Get-ChildItem ".local/generated/$role" -Filter '*_dispatch.c'
    foreach ($source in $sources) {
        $object = Join-Path $target ($source.BaseName + '.obj')
        $log = Join-Path $target ($source.BaseName + '.log')
        # Windows PowerShell reports native stderr as ErrorRecord objects,
        # including ordinary compiler warnings. Exit status is authoritative.
        $ErrorActionPreference = 'Continue'
        & clang -std=c99 -O1 -Wno-c23-extensions -DPSX_NO_DEBUG_TOOLS `
            -Itools/psxrecomp-src/runtime/include -c $source.FullName -o $object *> $log
        $compileExit = $LASTEXITCODE
        $ErrorActionPreference = 'Stop'
        if ($compileExit -ne 0) { throw "Object compilation failed: $($source.Name); see $log" }
    }
    $codegen = Get-Content ".local/codegen-$role.log" -Raw
    $count = [regex]::Match($codegen, 'Dispatch table (?:written|unchanged) \((\d+) entries\)').Groups[1].Value
    if (-not $count) { throw "Missing dispatch summary for $role" }
    $warnings = @([regex]::Matches($codegen, '(?m)^.*WARNING:.*$') | ForEach-Object { $_.Value.Trim() })
    $results += [pscustomobject]@{
        module = $role
        generated_functions = [int]$count
        compiled_objects = $sources.Count
        warnings = $warnings
    }
    Write-Output "$role`: compiled $($sources.Count) objects, $count dispatch entries, $($warnings.Count) code-generation warnings"
}
$report = [pscustomobject]@{
    stage = 'Isolated code generation and object compilation; no runtime link or game execution'
    framework_commit = (& git -C tools/psxrecomp-src rev-parse HEAD)
    compiler = (& clang --version | Select-Object -First 1)
    modules = $results
}
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath analysis/recomp-experiment.json -Encoding utf8
