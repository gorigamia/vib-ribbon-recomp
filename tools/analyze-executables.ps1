$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
foreach ($name in @('SCES_028.73', 'MAIN_G.EXE', 'MAIN_K.EXE', 'MAIN_T.EXE')) {
    & .local/bin/psxrecomp-analyze.exe ".local/executables/$name" `
        --out ".local/analysis/$name" `
        --emit-tsv ".local/analysis/$name/functions.tsv" `
        --disasm ".local/analysis/$name/disasm.txt" --top 5 *> ".local/$name-analysis.log"
    if ($LASTEXITCODE -ne 0) { throw "Analysis failed: $name" }
    Write-Output "Analyzed $name"
}
py -3 tools/summarize-analysis.py
if ($LASTEXITCODE -ne 0) { throw 'Summary failed' }

