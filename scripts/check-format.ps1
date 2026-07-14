# Pre-commit dry-run check. Exits 1 if any tracked file would change.
# Requires clang-format on PATH.
$ErrorActionPreference = 'Stop'
$bad = @()
git ls-files |
    Where-Object { $_ -match '\.(cpp|h|hpp|cc|cxx)$' } |
    ForEach-Object {
        $diff = clang-format --dry-run --Werror $_ 2>&1
        if ($LASTEXITCODE -ne 0) { $bad += $_ }
    }
if ($bad.Count -gt 0) {
    Write-Host "clang-format would change:" -ForegroundColor Red
    $bad | ForEach-Object { Write-Host "  $_" }
    exit 1
}
Write-Host "clang-format: OK"