# Format every tracked file in-place using clang-format.
# Requires clang-format on PATH.
$ErrorActionPreference = 'Stop'
git ls-files |
    Where-Object { $_ -match '\.(cpp|h|hpp|cc|cxx)$' } |
    ForEach-Object { clang-format -i $_ }