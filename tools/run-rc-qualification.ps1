param(
  [string]$BuildRoot = "./build",
  [string]$EvidenceDir = "./build/QualificationEvidence",
  [string]$ControllerModel = "",
  [switch]$ControllerTraversalPassed,
  [switch]$OverlayFocusPassed,
  [switch]$ForegroundRecoveryPassed,
  [switch]$RestartPersistencePassed,
  [switch]$FirstBootPassed,
  [switch]$UninstallReinstallPassed,
  [switch]$CrashContainmentPassed,
  [switch]$ResumeRoundTripPassed,
  [switch]$CanonicalStatePassed,
  [switch]$NonInteractiveValidation
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Resolve-RequiredFile([string]$relative) {
  $path = Join-Path $BuildRoot $relative
  if (-not (Test-Path $path -PathType Leaf)) { throw "Required file missing: $path" }
  if ((Get-Item $path).Length -le 0) { throw "Required file is empty: $path" }
  return (Resolve-Path $path).Path
}

function Read-JsonProcess([string]$exe) {
  $text = (& $exe 2>&1 | ForEach-Object { $_.ToString() }) -join "`n"
  if ($LASTEXITCODE -ne 0) { throw "Qualification probe failed with exit code $LASTEXITCODE. Output: $text" }
  return ($text | ConvertFrom-Json)
}

New-Item -ItemType Directory -Force -Path $EvidenceDir | Out-Null

$probeExe = Resolve-RequiredFile "Release\ZeroQualificationProbe.exe"
$zeroAcceptance = Resolve-RequiredFile "Release\ZeroAcceptance.exe"
$zeroShell = Resolve-RequiredFile "Release\ZeroVirtualConsole.exe"
$referenceExe = Resolve-RequiredFile "ReferencePackage\ZeroReferenceGame.exe"
$automatedJson = Join-Path $BuildRoot "AcceptanceReports\m1-automated-acceptance.json"
$automatedMd = Join-Path $BuildRoot "AcceptanceReports\m1-automated-acceptance.md"

if (-not (Test-Path $automatedJson -PathType Leaf)) { throw "M1 automated JSON evidence is missing: $automatedJson" }
if (-not (Test-Path $automatedMd -PathType Leaf)) { throw "M1 automated Markdown evidence is missing: $automatedMd" }

$automated = Get-Content $automatedJson -Raw | ConvertFrom-Json
if ($automated.automated_result -ne "PASS") { throw "M1 automated acceptance has not passed." }
if ($automated.rc_qualified -ne $false) { throw "Automated evidence must never self-qualify the RC." }

$probe = Read-JsonProcess $probeExe
$windows11x64 = $probe.windows_11 -eq $true -and $probe.architecture -eq "x64"
$physicalControllerDetected = [int]$probe.xinput_connected_count -gt 0

$manualChecks = [ordered]@{
  first_boot = [bool]$FirstBootPassed
  controller_traversal = [bool]$ControllerTraversalPassed
  overlay_focus = [bool]$OverlayFocusPassed
  foreground_recovery = [bool]$ForegroundRecoveryPassed
  restart_persistence = [bool]$RestartPersistencePassed
  uninstall_reinstall_preservation = [bool]$UninstallReinstallPassed
  crash_containment = [bool]$CrashContainmentPassed
  resume_roundtrip = [bool]$ResumeRoundTripPassed
  canonical_state = [bool]$CanonicalStatePassed
}

$manualAllPassed = $true
foreach ($entry in $manualChecks.GetEnumerator()) {
  if (-not $entry.Value) { $manualAllPassed = $false }
}

if ($NonInteractiveValidation) {
  # CI validates report semantics only. It must remain physically unqualified.
  $manualAllPassed = $false
}

$controllerIdentityPresent = -not [string]::IsNullOrWhiteSpace($ControllerModel)
$qualified =
  $automated.automated_result -eq "PASS" -and
  $windows11x64 -and
  $physicalControllerDetected -and
  $controllerIdentityPresent -and
  $manualAllPassed -and
  -not $NonInteractiveValidation

$commit = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else {
  try { (& git rev-parse HEAD 2>$null).Trim() } catch { "unknown" }
}

$report = [ordered]@{
  schema = 1
  generated_at_utc = [DateTime]::UtcNow.ToString("o")
  zero_milestone = "M1 / Runtime V4.1"
  version = "0.1.0-rc1"
  commit = $commit
  rc_qualified = [bool]$qualified
  qualification_result = if ($qualified) { "PASS" } else { "PENDING" }
  automated_result = $automated.automated_result
  machine = [ordered]@{
    name = $probe.machine_name
    windows_11 = [bool]$probe.windows_11
    os_build = [int]$probe.os_build
    architecture = $probe.architecture
  }
  controller = [ordered]@{
    model = $ControllerModel
    xinput_connected_count = [int]$probe.xinput_connected_count
    xinput_slots = @($probe.xinput_slots)
    detected = [bool]$physicalControllerDetected
  }
  manual_checks = $manualChecks
  evidence = [ordered]@{
    automated_json = (Resolve-Path $automatedJson).Path
    automated_markdown = (Resolve-Path $automatedMd).Path
    zero_acceptance = $zeroAcceptance
    zero_shell = $zeroShell
    reference_game = $referenceExe
  }
  blockers = @()
}

$blockers = New-Object System.Collections.Generic.List[string]
if (-not $windows11x64) { $blockers.Add("Qualification requires a real Windows 11 x64 machine.") }
if (-not $physicalControllerDetected) { $blockers.Add("No physical XInput controller was detected.") }
if (-not $controllerIdentityPresent) { $blockers.Add("Controller make/model was not recorded.") }
foreach ($entry in $manualChecks.GetEnumerator()) {
  if (-not $entry.Value) { $blockers.Add("Manual qualification check not passed: $($entry.Key)") }
}
if ($NonInteractiveValidation) { $blockers.Add("CI/non-interactive validation can never produce RC qualification.") }
$report.blockers = @($blockers)

$jsonPath = Join-Path $EvidenceDir "rc-qualification.json"
$mdPath = Join-Path $EvidenceDir "rc-qualification.md"
$report | ConvertTo-Json -Depth 8 | Set-Content -Path $jsonPath -Encoding UTF8

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# ZERO Runtime V4.1 RC Qualification")
$lines.Add("")
$lines.Add("- Version: **0.1.0-rc1**")
$lines.Add("- Commit: ``$commit``")
$lines.Add("- Qualification result: **$($report.qualification_result)**")
$lines.Add("- RC qualified: **$(if ($qualified) { 'YES' } else { 'NO' })**")
$lines.Add("- Windows build: $($probe.os_build) ($($probe.architecture))")
$lines.Add("- Controller: $(if ($ControllerModel) { $ControllerModel } else { 'not recorded' })")
$lines.Add("")
$lines.Add("## Manual qualification checks")
$lines.Add("")
foreach ($entry in $manualChecks.GetEnumerator()) {
  $lines.Add("- $($entry.Key): **$(if ($entry.Value) { 'PASS' } else { 'PENDING' })**")
}
$lines.Add("")
$lines.Add("## Blockers")
$lines.Add("")
if ($blockers.Count -eq 0) {
  $lines.Add("- None. Qualification evidence is complete.")
} else {
  foreach ($item in $blockers) { $lines.Add("- $item") }
}
$lines | Set-Content -Path $mdPath -Encoding UTF8

Write-Host "ZERO RC qualification JSON: $jsonPath"
Write-Host "ZERO RC qualification Markdown: $mdPath"
Write-Host "ZERO RC qualification result: $($report.qualification_result)"

if ($NonInteractiveValidation) {
  if ($qualified) { throw "CI validation must never mark an RC qualified." }
  if ($report.qualification_result -ne "PENDING") { throw "CI qualification result must remain PENDING." }
  exit 0
}

if (-not $qualified) { exit 2 }
exit 0
