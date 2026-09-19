param(
  [string]$BuildRoot = "./build",
  [string]$EvidenceDir = "./build/QualificationEvidence",
  [string]$ControllerModel = "",
  [string]$InstalledAcceptancePath = "$env:LOCALAPPDATA\Programs\ZERO Virtual Console\ZeroAcceptance.exe",
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
$buildAcceptance = Resolve-RequiredFile "Release\ZeroAcceptance.exe"
$zeroShell = Resolve-RequiredFile "Release\ZeroVirtualConsole.exe"
$referenceExe = Resolve-RequiredFile "ReferencePackage\ZeroReferenceGame.exe"
$automatedJson = Join-Path $BuildRoot "AcceptanceReports\m1-automated-acceptance.json"
$automatedMd = Join-Path $BuildRoot "AcceptanceReports\m1-automated-acceptance.md"
$architectureJson = Join-Path $BuildRoot "ArchitectureReports\production-architecture.json"
$architectureMd = Join-Path $BuildRoot "ArchitectureReports\production-architecture.md"

foreach ($path in @($automatedJson, $automatedMd, $architectureJson, $architectureMd)) {
  if (-not (Test-Path $path -PathType Leaf)) { throw "Required qualification evidence is missing: $path" }
  if ((Get-Item $path).Length -le 0) { throw "Required qualification evidence is empty: $path" }
}

$automated = Get-Content $automatedJson -Raw | ConvertFrom-Json
$architecture = Get-Content $architectureJson -Raw | ConvertFrom-Json
if ($automated.automated_result -ne "PASS") { throw "M1 automated acceptance has not passed." }
if ($automated.rc_qualified -ne $false) { throw "Automated evidence must never self-qualify the RC." }
if ($architecture.result -ne "PASS") { throw "Production architecture gate has not passed." }

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
if ($NonInteractiveValidation) { $manualAllPassed = $false }

$installedAcceptancePassed = $false
$installedAcceptanceOutput = "Not executed in CI/non-interactive validation."
$resolvedInstalledAcceptance = $null
if (-not $NonInteractiveValidation) {
  if (Test-Path $InstalledAcceptancePath -PathType Leaf) {
    $resolvedInstalledAcceptance = (Resolve-Path $InstalledAcceptancePath).Path
    $captured = (& $resolvedInstalledAcceptance 2>&1 | ForEach-Object { $_.ToString() })
    $installedAcceptanceExit = $LASTEXITCODE
    $installedAcceptanceOutput = $captured -join "`n"
    $installedAcceptancePassed = $installedAcceptanceExit -eq 0
  } else {
    $installedAcceptanceOutput = "Installed ZeroAcceptance.exe was not found at $InstalledAcceptancePath"
  }
}

$controllerIdentityPresent = -not [string]::IsNullOrWhiteSpace($ControllerModel)
$qualified =
  $architecture.result -eq "PASS" -and
  $automated.automated_result -eq "PASS" -and
  $windows11x64 -and
  $physicalControllerDetected -and
  $controllerIdentityPresent -and
  $manualAllPassed -and
  $installedAcceptancePassed -and
  -not $NonInteractiveValidation

$commit = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else {
  try { (& git rev-parse HEAD 2>$null).Trim() } catch { "unknown" }
}

$blockers = New-Object System.Collections.Generic.List[string]
if ($architecture.result -ne "PASS") { [void]$blockers.Add("Production architecture gate has not passed.") }
if (-not $windows11x64) { [void]$blockers.Add("Qualification requires a real Windows 11 x64 machine.") }
if (-not $physicalControllerDetected) { [void]$blockers.Add("No physical XInput controller was detected.") }
if (-not $controllerIdentityPresent) { [void]$blockers.Add("Controller make/model was not recorded.") }
foreach ($entry in $manualChecks.GetEnumerator()) {
  if (-not $entry.Value) { [void]$blockers.Add("Manual qualification check not passed: $($entry.Key)") }
}
if (-not $NonInteractiveValidation -and -not $installedAcceptancePassed) {
  [void]$blockers.Add("Installed ZeroAcceptance.exe did not complete with PASS/exit 0.")
}
if ($NonInteractiveValidation) { [void]$blockers.Add("CI/non-interactive validation can never produce RC qualification.") }
$blockerArray = [string[]]$blockers

$shellHash = (Get-FileHash -Algorithm SHA256 -Path $zeroShell).Hash.ToLowerInvariant()
$referenceHash = (Get-FileHash -Algorithm SHA256 -Path $referenceExe).Hash.ToLowerInvariant()

$report = [ordered]@{
  schema = 2
  generated_at_utc = [DateTime]::UtcNow.ToString("o")
  zero_milestone = "M1 / ZERO Core V5 / Production Architecture"
  version = "0.1.0-rc1"
  commit = $commit
  rc_qualified = [bool]$qualified
  qualification_result = if ($qualified) { "PASS" } else { "PENDING" }
  architecture_result = $architecture.result
  automated_result = $automated.automated_result
  architecture_metrics = $architecture.source_metrics
  automated_performance_metrics = $automated.performance_metrics
  candidate = [ordered]@{
    shell_sha256 = $shellHash
    reference_game_sha256 = $referenceHash
  }
  machine = [ordered]@{
    windows_11 = [bool]$probe.windows_11
    os_build = [int]$probe.os_build
    architecture = $probe.architecture
  }
  controller = [ordered]@{
    model = $ControllerModel
    xinput_connected_count = [int]$probe.xinput_connected_count
    xinput_slots = [bool[]]$probe.xinput_slots
    detected = [bool]$physicalControllerDetected
  }
  installed_acceptance = [ordered]@{
    path = $resolvedInstalledAcceptance
    passed = [bool]$installedAcceptancePassed
    output = $installedAcceptanceOutput
  }
  manual_checks = $manualChecks
  evidence = [ordered]@{
    architecture_json = (Resolve-Path $architectureJson).Path
    architecture_markdown = (Resolve-Path $architectureMd).Path
    automated_json = (Resolve-Path $automatedJson).Path
    automated_markdown = (Resolve-Path $automatedMd).Path
    build_acceptance = $buildAcceptance
    zero_shell = $zeroShell
    reference_game = $referenceExe
  }
  blockers = $blockerArray
}

$jsonPath = Join-Path $EvidenceDir "rc-qualification.json"
$mdPath = Join-Path $EvidenceDir "rc-qualification.md"
$report | ConvertTo-Json -Depth 10 | Set-Content -Path $jsonPath -Encoding UTF8

$lines = New-Object System.Collections.Generic.List[string]
[void]$lines.Add("# ZERO Core V5 RC Qualification")
[void]$lines.Add("")
[void]$lines.Add("- Version: **0.1.0-rc1**")
[void]$lines.Add("- Commit: ``$commit``")
[void]$lines.Add("- Qualification result: **$($report.qualification_result)**")
[void]$lines.Add("- RC qualified: **$(if ($qualified) { 'YES' } else { 'NO' })**")
[void]$lines.Add("- Architecture gate: **$($architecture.result)**")
[void]$lines.Add("- Automated acceptance: **$($automated.automated_result)**")
[void]$lines.Add("- Shell SHA-256: ``$shellHash``")
[void]$lines.Add("- Reference game SHA-256: ``$referenceHash``")
[void]$lines.Add("- Windows build: $($probe.os_build) ($($probe.architecture))")
[void]$lines.Add("- Controller: $(if ($ControllerModel) { $ControllerModel } else { 'not recorded' })")
[void]$lines.Add("- Installed acceptance: **$(if ($installedAcceptancePassed) { 'PASS' } else { 'PENDING' })**")
[void]$lines.Add("")
[void]$lines.Add("## Manual qualification checks")
[void]$lines.Add("")
foreach ($entry in $manualChecks.GetEnumerator()) {
  [void]$lines.Add("- $($entry.Key): **$(if ($entry.Value) { 'PASS' } else { 'PENDING' })**")
}
[void]$lines.Add("")
[void]$lines.Add("## Blockers")
[void]$lines.Add("")
if ($blockers.Count -eq 0) {
  [void]$lines.Add("- None. Qualification evidence is complete.")
} else {
  foreach ($item in $blockers) { [void]$lines.Add("- $item") }
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
