param(
  [string]$SourceRoot = ".",
  [string]$ReportDir = "./build/GovernanceReports"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
$violations = New-Object System.Collections.Generic.List[object]
function Violate([string]$rule,[string]$path,[string]$detail) { $violations.Add([ordered]@{rule=$rule;path=$path;detail=$detail}) }

$requiredFiles = @(
  ".github/CODEOWNERS",
  ".github/workflows/windows-build.yml",
  ".github/workflows/codeql.yml",
  ".github/workflows/production-release.yml",
  "SECURITY.md",
  "docs/SERVER_AUTHORITY_SECURITY.md",
  "tools/generate-sbom.ps1",
  "tools/verify-authenticode.ps1",
  "tools/package-production-release.ps1"
)
foreach ($relative in $requiredFiles) {
  if (-not (Test-Path (Join-Path $SourceRoot $relative) -PathType Leaf)) { Violate "required_release_governance_file" $relative "Required production governance artifact missing." }
}

$workflowFiles = @(Get-ChildItem (Join-Path $SourceRoot ".github/workflows") -File -Include *.yml,*.yaml)
foreach ($workflow in $workflowFiles) {
  $text = Get-Content $workflow.FullName -Raw
  foreach ($match in [regex]::Matches($text, '(?m)^\s*uses:\s*([^\s#]+)')) {
    $use = $match.Groups[1].Value
    if ($use -match '@(v\d+|main|master|latest)$') { Violate "immutable_action_pin" $workflow.Name "Mutable GitHub Action reference: $use" }
    if ($use -notmatch '@[0-9a-fA-F]{40}$') { Violate "immutable_action_pin" $workflow.Name "GitHub Action must be pinned to a 40-character commit SHA: $use" }
  }
}

$production = Get-Content (Join-Path $SourceRoot ".github/workflows/production-release.yml") -Raw
$signatureGateIndex = $production.IndexOf("Verify Authenticode signature")
$signedSbomIndex = $production.IndexOf("Generate signed-artifact SPDX SBOM")
$packageIndex = $production.IndexOf("Build production release bundle")
if ($signatureGateIndex -lt 0 -or $signedSbomIndex -lt 0 -or $packageIndex -lt 0 -or
    $signedSbomIndex -le $signatureGateIndex -or $packageIndex -le $signedSbomIndex) {
  Violate "signed_artifact_sbom_order" ".github/workflows/production-release.yml" "Production SBOM must be generated after Authenticode verification and before packaging."
}

foreach ($required in @(
  "environment: production",
  "Verify repository governance",
  "Validate release governance and supply-chain controls",
  "Validate server-authority fail-closed contract",
  "Validate production shell navigation contract",
  "Validate Friends experience contract",
  "Validate Captures experience contract",
  "Validate Home Library and Game Detail contract",
  "Validate Store Settings and First Boot contract",
  "Validate persisted First Boot recovery",
  "ZERO_PHYSICAL_QUALIFICATION_B64",
  "candidate.shell_sha256",
  "Production shell bytes do not match the exact physically qualified shell artifact.",
  "ZERO_SIGNING_PFX_BASE64",
  "verify-authenticode.ps1",
  "generate-sbom.ps1",
  "package-production-release.ps1",
  "main is not branch-protected"
)) {
  if ($production -notmatch [regex]::Escape($required)) { Violate "production_release_fail_closed" ".github/workflows/production-release.yml" "Missing required release control: $required" }
}

$serverAuthority = Get-Content (Join-Path $SourceRoot "docs/SERVER_AUTHORITY_SECURITY.md") -Raw
foreach ($required in @("never a trusted authority","entitlements","Friends graph","replay","rate limiting","fails closed")) {
  if ($serverAuthority -notmatch [regex]::Escape($required)) { Violate "server_authority_standard" "docs/SERVER_AUTHORITY_SECURITY.md" "Missing authority requirement: $required" }
}

$standard = Get-Content (Join-Path $SourceRoot "docs/PRODUCTION_ARCHITECTURE_STANDARD.md") -Raw
foreach ($required in @("Production binaries are signed","SPDX SBOM","Online/commercial state is server-authoritative","Main-branch governance is mandatory")) {
  if ($standard -notmatch [regex]::Escape($required)) { Violate "production_standard_release_controls" "docs/PRODUCTION_ARCHITECTURE_STANDARD.md" "Missing production standard control: $required" }
}

$status = if ($violations.Count -eq 0) { "PASS" } else { "FAIL" }
$commit = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else { try { (& git rev-parse HEAD 2>$null).Trim() } catch { "unknown" } }
$report = [ordered]@{
  schema = 1
  generated_at_utc = [DateTime]::UtcNow.ToString("o")
  commit = $commit
  result = $status
  violations = $violations.ToArray()
}
$json = Join-Path $ReportDir "release-governance.json"
$md = Join-Path $ReportDir "release-governance.md"
$report | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 -Path $json
$lines = @("# ZERO Release Governance Report","","- Commit: ``$commit``","- Result: **$status**","","## Violations")
if ($violations.Count -eq 0) { $lines += "None." } else { foreach ($v in $violations) { $lines += "- **$($v.rule)** — ``$($v.path)`` — $($v.detail)" } }
$lines | Set-Content -Encoding UTF8 -Path $md
if ($violations.Count -gt 0) { foreach ($v in $violations) { Write-Host "GOVERNANCE VIOLATION: $($v.rule) | $($v.path) | $($v.detail)" }; exit 2 }
Write-Host "ZERO release governance: PASS"
