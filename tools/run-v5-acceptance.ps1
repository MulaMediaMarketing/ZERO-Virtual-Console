param(
    [string]$BuildRoot = "./build"
)

$ErrorActionPreference = "Stop"
$release = Join-Path $BuildRoot "Release"
if (-not (Test-Path $release)) {
    throw "V5 acceptance release directory not found: $release"
}

$tests = Get-ChildItem -Path $release -Filter "ZeroV5*Acceptance.exe" -File | Sort-Object Name
if (-not $tests -or $tests.Count -eq 0) {
    throw "No ZERO V5 acceptance executables were built."
}

foreach ($test in $tests) {
    Write-Host "[V5] $($test.Name)"
    & $test.FullName
    if ($LASTEXITCODE -ne 0) {
        throw "$($test.Name) failed with exit code $LASTEXITCODE"
    }
}

Write-Host "ZERO V5 migration acceptance: PASS ($($tests.Count) executables)"
