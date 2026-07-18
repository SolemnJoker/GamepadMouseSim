# Pre-commit dry-run check. Exits 1 if any tracked file would change.
# Requires clang-format on PATH. Falls back to standard LLVM install
# path if not on PATH.
$ErrorActionPreference = 'Stop'

# Locate clang-format
$clangFormat = (Get-Command clang-format -ErrorAction SilentlyContinue)
if (-not $clangFormat) {
    $candidates = @(
        'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\bin\clang-format.exe',
        'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\Llvm\bin\clang-format.exe',
        'C:\Program Files\LLVM\bin\clang-format.exe'
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { $clangFormat = @{Path = $c}; break }
    }
}
if (-not $clangFormat) {
    Write-Error "clang-format not found. Install LLVM or add to PATH."
    exit 2
}

$bad = @()
git ls-files |
    Where-Object { $_ -match '\.(cpp|h|hpp|cc|cxx)$' } |
    ForEach-Object {
        & $clangFormat.Path --dry-run --Werror $_ 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) { $bad += $_ }
    }
if ($bad.Count -gt 0) {
    Write-Host "clang-format would change:" -ForegroundColor Red
    $bad | ForEach-Object { Write-Host "  $_" }
    exit 1
}
Write-Host "clang-format: OK"