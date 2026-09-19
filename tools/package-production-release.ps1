param(
  [string]$BuildRoot = "./build",
  [string]$OutputDir = "./build/ProductionRelease",
  [string]$QualificationEvidence = "./build/QualificationEvidence/rc-qualification.json",
  [string]$SbomPath = "./build/SBOM/zero-virtual-console.spdx.json",
  [string]$SigningReport = "./build/SigningReports/authenticode.json"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
function Require-File([string]$Path) {
  if (-not (Test-Path $Path -PathType Leaf)) { throw "Missing production release file: $Path" }
  if ((Get-Item $Path).Length -le 0) { throw "Empty production release file: $Path" }
}

$architecturePath = Join-Path $BuildRoot "ArchitectureReports/production-architecture.json"
$acceptancePath = Join-Path $BuildRoot "AcceptanceReports/m1-automated-acceptance.json"
$dependencyPath = Join-Path $BuildRoot "DependencyReports/dependency-surface.json"
$releaseExe = Join-Path $BuildRoot "Release/ZeroVirtualConsole.exe"
foreach ($p in @($architecturePath,$acceptancePath,$dependencyPath,$QualificationEvidence,$SbomPath,$SigningReport,$releaseExe)) { Require-File $p }

$architecture = Get-Content $architecturePath -Raw | ConvertFrom-Json
$acceptance = Get-Content $acceptancePath -Raw | ConvertFrom-Json
$dependency = Get-Content $dependencyPath -Raw | ConvertFrom-Json
$qualification = Get-Content $QualificationEvidence -Raw | ConvertFrom-Json
$signing = Get-Content $SigningReport -Raw | ConvertFrom-Json

if ($architecture.result -ne "PASS") { throw "Architecture gate is not PASS." }
if ($acceptance.automated_result -ne "PASS") { throw "Automated M1 acceptance is not PASS." }
if ($dependency.result -ne "PASS") { throw "Dependency surface is not PASS." }
if ($signing.result -ne "PASS") { throw "Authenticode verification is not PASS." }
if ($qualification.rc_qualified -ne $true) { throw "Production release requires physical RC qualification." }

$commit = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else { (& git rev-parse HEAD).Trim() }
if (-not $commit) { throw "Unable to resolve release commit." }
$qualifiedCommit = if ($qualification.PSObject.Properties.Name -contains "commit") { [string]$qualification.commit } elseif ($qualification.PSObject.Properties.Name -contains "candidate_commit") { [string]$qualification.candidate_commit } else { "" }
if (-not $qualifiedCommit) { throw "Qualification evidence does not identify an exact commit." }
if ($qualifiedCommit.ToLowerInvariant() -ne $commit.ToLowerInvariant()) { throw "Qualification evidence belongs to $qualifiedCommit, not exact release commit $commit." }

$sig = Get-AuthenticodeSignature -FilePath $releaseExe
if ($sig.Status -ne [System.Management.Automation.SignatureStatus]::Valid) { throw "ZeroVirtualConsole.exe is not validly Authenticode signed." }

if (Test-Path $OutputDir) { Remove-Item -Recurse -Force $OutputDir }
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$payload = @(
  @{ source=$releaseExe; relative="ZeroVirtualConsole.exe" },
  @{ source=$SbomPath; relative="zero-virtual-console.spdx.json" },
  @{ source=$architecturePath; relative="evidence/production-architecture.json" },
  @{ source=$acceptancePath; relative="evidence/m1-automated-acceptance.json" },
  @{ source=$dependencyPath; relative="evidence/dependency-surface.json" },
  @{ source=$QualificationEvidence; relative="evidence/rc-qualification.json" },
  @{ source=$SigningReport; relative="evidence/authenticode.json" }
)

$entries = New-Object System.Collections.Generic.List[object]
foreach ($item in $payload) {
  $dest = Join-Path $OutputDir $item.relative
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $dest) | Out-Null
  Copy-Item -Force $item.source $dest
  $entries.Add([ordered]@{
    path = $item.relative.Replace('\\','/')
    sha256 = (Get-FileHash -Algorithm SHA256 $dest).Hash.ToLowerInvariant()
    bytes = (Get-Item $dest).Length
  })
}

$manifest = [ordered]@{
  schema = 2
  product = "ZERO Player"
  platform = "ZERO Core V5"
  release_class = "production"
  runtime = "V5"
  commit = $commit
  generated_at_utc = [DateTime]::UtcNow.ToString("o")
  architecture_result = $architecture.result
  automated_acceptance_result = $acceptance.automated_result
  dependency_result = $dependency.result
  signing_result = $signing.result
  rc_qualified = $true
  signer_subject = $sig.SignerCertificate.Subject
  signer_thumbprint = $sig.SignerCertificate.Thumbprint
  files = $entries.ToArray()
}
$manifestPath = Join-Path $OutputDir "production-release-manifest.json"
$manifest | ConvertTo-Json -Depth 12 | Set-Content -Encoding UTF8 -Path $manifestPath

$zipPath = Join-Path (Split-Path -Parent $OutputDir) "ZERO-Player-Windows-x64-Production.zip"
if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
Compress-Archive -Path (Join-Path $OutputDir '*') -DestinationPath $zipPath -CompressionLevel Optimal
Require-File $zipPath
Write-Host "ZERO production release bundle: $zipPath"
