param(
  [string]$BuildRoot = "./build"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$required = @(
  (Join-Path $BuildRoot "Release\ZeroVirtualConsole.exe"),
  (Join-Path $BuildRoot "Release\ZeroAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroIpcPersistenceAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroPackageIntegrityAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroPackageTrustAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroPackageRepairAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroStrictJsonAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroPlatformDatabaseAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroFaultInjectionAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroProductionBenchmarkAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroInputAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroShellUxAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroProductionUxAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroFriendsExperienceAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroCapturesExperienceAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroCoreShellExperienceAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroStoreSettingsFirstBootAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroFirstBootPersistenceAcceptance.exe"),
  (Join-Path $BuildRoot "Release\ZeroReferenceGame.exe"),
  (Join-Path $BuildRoot "Release\ZeroQualificationProbe.exe"),
  (Join-Path $BuildRoot "ReferencePackage\ZeroReferenceGame.exe"),
  (Join-Path $BuildRoot "ReferencePackage\zero.manifest.json"),
  (Join-Path $BuildRoot "ReferencePackage\zero.integrity.sha256"),
  (Join-Path $BuildRoot "ReferencePackage\Assets\hero.png"),
  (Join-Path $BuildRoot "ReferencePackage\Assets\icon.png"),
  (Join-Path $BuildRoot "ReferencePackage\Assets\logo.png"),
  "./installer/install-zero.ps1",
  "./installer/uninstall-zero.ps1",
  "./tools/verify-installer-v2.ps1",
  "./tools/verify-production-architecture.ps1",
  "./tools/verify-m1-automated.ps1",
  "./tools/run-rc-qualification.ps1",
  "./tools/package-release.ps1",
  "./docs/PRODUCTION_ARCHITECTURE_STANDARD.md",
  "./docs/RUNTIME_V4_1_RC_GATE.md"
)

$missing = @()
foreach ($path in $required) {
  if (-not (Test-Path $path -PathType Leaf)) { $missing += $path }
}
if ($missing.Count -gt 0) { throw "ZERO RC artifact verification failed. Missing: $($missing -join ', ')" }

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
Write-Host "Production architecture, security, persistence, fault-injection, performance, and UX acceptance payloads are present."
Write-Host "Physical qualification is still required before rc_qualified may become true."
