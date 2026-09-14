param(
  [string]$BuildRoot = "./build",
  [string]$OutputDir = "./build/ReleaseBundle"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Require-File([string]$Path) {
  if (-not (Test-Path $Path -PathType Leaf)) { throw "Missing release file: $Path" }
  if ((Get-Item $Path).Length -le 0) { throw "Empty release file: $Path" }
}

$files = @(
  "Release/ZeroVirtualConsole.exe",
  "Release/ZeroTestGame.exe",
  "Release/ZeroReferenceGame.exe",
  "Release/ZeroAcceptance.exe",
  "Release/ZeroIpcPersistenceAcceptance.exe",
  "Release/ZeroPackageIntegrityAcceptance.exe",
  "Release/ZeroPackageTrustAcceptance.exe",
  "Release/ZeroPackageRepairAcceptance.exe",
  "Release/ZeroInputAcceptance.exe",
  "Release/ZeroShellUxAcceptance.exe",
  "Release/ZeroProductionUxAcceptance.exe",
  "Release/ZeroFriendsExperienceAcceptance.exe",
  "Release/ZeroCapturesExperienceAcceptance.exe",
  "Release/ZeroCoreShellExperienceAcceptance.exe",
  "Release/ZeroStoreSettingsFirstBootAcceptance.exe",
  "Release/ZeroQualificationProbe.exe",
  "ReferencePackage/ZeroReferenceGame.exe",
  "ReferencePackage/zero.manifest.json",
  "ReferencePackage/zero.integrity.sha256",
  "AcceptanceReports/m1-automated-acceptance.json",
  "AcceptanceReports/m1-automated-acceptance.md",
  "QualificationEvidence/rc-qualification.json",
  "QualificationEvidence/rc-qualification.md"
)

if (Test-Path $OutputDir) { Remove-Item -Recurse -Force $OutputDir }
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$manifestEntries = New-Object System.Collections.Generic.List[object]
foreach ($relative in $files) {
  $source = Join-Path $BuildRoot $relative
  Require-File $source
  $destination = Join-Path $OutputDir $relative
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
  Copy-Item -Force $source $destination
  $hash = (Get-FileHash -Algorithm SHA256 $destination).Hash.ToLowerInvariant()
  $manifestEntries.Add([pscustomobject]@{
    path = $relative.Replace('\\','/')
    sha256 = $hash
    bytes = (Get-Item $destination).Length
  })
}

$commit = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else {
  try { (& git rev-parse HEAD 2>$null).Trim() } catch { "unknown" }
}

$manifest = [ordered]@{
  schema = 1
  product = "ZERO Virtual Console"
  milestone = "M1 / Runtime V4.1 / Production UX"
  commit = $commit
  generated_at_utc = [DateTime]::UtcNow.ToString("o")
  rc_qualified = $false
  rc_qualification_note = "Bundle is release-candidate material only until physical Windows 11 x64 and controller qualification passes."
  files = $manifestEntries.ToArray()
}

$manifestPath = Join-Path $OutputDir "release-manifest.json"
$manifest | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 -Path $manifestPath

$zipPath = Join-Path (Split-Path -Parent $OutputDir) "ZERO-Virtual-Console-Windows-x64-RC.zip"
if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
Compress-Archive -Path (Join-Path $OutputDir '*') -DestinationPath $zipPath -CompressionLevel Optimal
Require-File $zipPath

Write-Host "ZERO release bundle: $OutputDir"
Write-Host "ZERO release manifest: $manifestPath"
Write-Host "ZERO RC archive: $zipPath"
Write-Host "ZERO RC qualification: PENDING PHYSICAL ACCEPTANCE"
