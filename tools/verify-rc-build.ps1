param(
  [string]$BuildRoot = "./build"
)

$ErrorActionPreference = "Stop"

$required = @(
  (Join-Path $BuildRoot "Release\ZeroVirtualConsole.exe"),
  (Join-Path $BuildRoot "Release\ZeroAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroReferenceGame.exe"),
  (Join-Path $BuildRoot "ReferencePackage\ZeroReferenceGame.exe"),
  (Join-Path $BuildRoot "ReferencePackage\zero.manifest.json"),
  (Join-Path $BuildRoot "ReferencePackage\Assets\hero.png"),
  (Join-Path $BuildRoot "ReferencePackage\Assets\icon.png"),
  (Join-Path $BuildRoot "ReferencePackage\Assets\logo.png"),
  "./installer/install-zero.ps1",
  "./installer/uninstall-zero.ps1"
)

$missing = @()
foreach ($path in $required) {
  if (-not (Test-Path $path -PathType Leaf)) { $missing += $path }
}
if ($missing.Count -gt 0) {
  throw "ZERO RC artifact verification failed. Missing: $($missing -join ', ')"
}

$manifestPath = Join-Path $BuildRoot "ReferencePackage\zero.manifest.json"
$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
if ($manifest.package_id -ne "zero.system.reference") { throw "Reference package_id is not locked." }
if ($manifest.minimum_runtime_major -ne 4) { throw "Reference package runtime contract is not V4." }
if ($manifest.zero_resume -ne $true) { throw "Reference package must enable ZERO Resume." }
if ($manifest.zero_achievements -ne $true) { throw "Reference package must enable ZERO achievements." }
if ($manifest.zero_overlay -ne $true) { throw "Reference package must enable ZERO overlay." }

foreach ($path in $required) {
  if ((Test-Path $path -PathType Leaf) -and ((Get-Item $path).Length -le 0)) {
    throw "ZERO RC artifact verification failed because '$path' is empty."
  }
}

Write-Host "ZERO Runtime V4.1 RC build payload verification: PASS"
