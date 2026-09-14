param(
  [string]$BuildRoot = "./build",
  [string]$ReportDir = "./build/AcceptanceReports"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null

$results = New-Object System.Collections.Generic.List[object]
$automatedRuntimeBudgetMs = 600000

function Resolve-RequiredFile([string]$RelativePath) {
  $path = Join-Path $BuildRoot $RelativePath
  if (-not (Test-Path $path -PathType Leaf)) {
    throw "Required acceptance artifact is missing: $path"
  }
  if ((Get-Item $path).Length -le 0) {
    throw "Required acceptance artifact is empty: $path"
  }
  return (Resolve-Path $path).Path
}

function Invoke-AcceptanceStep {
  param(
    [string]$Name,
    [string]$Category,
    [string]$Command,
    [string[]]$Arguments = @()
  )

  $started = [DateTime]::UtcNow
  $output = ""
  $exitCode = 1
  try {
    $captured = & $Command @Arguments 2>&1
    $exitCode = $LASTEXITCODE
    $output = ($captured | ForEach-Object { $_.ToString() }) -join "`n"
  } catch {
    $output = $_.Exception.Message
    $exitCode = 1
  }

  $ended = [DateTime]::UtcNow
  $passed = $exitCode -eq 0
  $status = if ($passed) { "PASS" } else { "FAIL" }
  $results.Add([pscustomobject]@{
    name = $Name
    category = $Category
    status = $status
    exit_code = $exitCode
    started_at_utc = $started.ToString("o")
    ended_at_utc = $ended.ToString("o")
    duration_ms = [int][Math]::Round(($ended - $started).TotalMilliseconds)
    output = $output
  })

  Write-Host ("[{0}] {1}" -f $status, $Name)
  if (-not $passed -and $output) { Write-Host $output }
}

$ipc = Resolve-RequiredFile "Release\ZeroIpcPersistenceAcceptance.exe"
$integrity = Resolve-RequiredFile "Release\ZeroPackageIntegrityAcceptance.exe"
$trust = Resolve-RequiredFile "Release\ZeroPackageTrustAcceptance.exe"
$repair = Resolve-RequiredFile "Release\ZeroPackageRepairAcceptance.exe"
$input = Resolve-RequiredFile "Release\ZeroInputAcceptance.exe"
$shellUx = Resolve-RequiredFile "Release\ZeroShellUxAcceptance.exe"
$productionUx = Resolve-RequiredFile "Release\ZeroProductionUxAcceptance.exe"
$friendsUx = Resolve-RequiredFile "Release\ZeroFriendsExperienceAcceptance.exe"
$capturesUx = Resolve-RequiredFile "Release\ZeroCapturesExperienceAcceptance.exe"
$coreShellUx = Resolve-RequiredFile "Release\ZeroCoreShellExperienceAcceptance.exe"
$storeSettingsFirstBootUx = Resolve-RequiredFile "Release\ZeroStoreSettingsFirstBootAcceptance.exe"
$firstBootPersistence = Resolve-RequiredFile "Release\ZeroFirstBootPersistenceAcceptance.exe"
$referenceExe = Resolve-RequiredFile "ReferencePackage\ZeroReferenceGame.exe"
$referenceManifest = Resolve-RequiredFile "ReferencePackage\zero.manifest.json"
$referenceIntegrity = Resolve-RequiredFile "ReferencePackage\zero.integrity.sha256"
$zeroShell = Resolve-RequiredFile "Release\ZeroVirtualConsole.exe"
$zeroAcceptance = Resolve-RequiredFile "Release\ZeroAcceptance.exe"

$referenceRoot = Split-Path -Parent $referenceExe
$installerGate = (Resolve-Path "./tools/verify-installer-v2.ps1").Path
$rcGate = (Resolve-Path "./tools/verify-rc-build.ps1").Path

Invoke-AcceptanceStep -Name "runtime_persistence_ack" -Category "runtime" -Command $ipc
Invoke-AcceptanceStep -Name "package_integrity" -Category "security" -Command $integrity -Arguments @($referenceRoot)
Invoke-AcceptanceStep -Name "package_trust_crypto" -Category "security" -Command $trust
Invoke-AcceptanceStep -Name "package_repair" -Category "installation" -Command $repair
Invoke-AcceptanceStep -Name "controller_input_contract" -Category "input" -Command $input
Invoke-AcceptanceStep -Name "shell_overlay_ux_contract" -Category "shell" -Command $shellUx
Invoke-AcceptanceStep -Name "production_navigation_contract" -Category "production_ux" -Command $productionUx
Invoke-AcceptanceStep -Name "friends_experience_contract" -Category "production_ux" -Command $friendsUx
Invoke-AcceptanceStep -Name "captures_experience_contract" -Category "production_ux" -Command $capturesUx
Invoke-AcceptanceStep -Name "core_shell_experience_contract" -Category "production_ux" -Command $coreShellUx
Invoke-AcceptanceStep -Name "store_settings_firstboot_contract" -Category "production_ux" -Command $storeSettingsFirstBootUx
Invoke-AcceptanceStep -Name "first_boot_persistence" -Category "persistence" -Command $firstBootPersistence
Invoke-AcceptanceStep -Name "installer_update_uninstall" -Category "installation" -Command "pwsh" -Arguments @("-NoProfile", "-File", $installerGate, "-BuildRoot", $BuildRoot)
Invoke-AcceptanceStep -Name "rc_payload" -Category "release" -Command "pwsh" -Arguments @("-NoProfile", "-File", $rcGate, "-BuildRoot", $BuildRoot)

$manifest = Get-Content $referenceManifest -Raw | ConvertFrom-Json
$integrityHeader = (Get-Content $referenceIntegrity -TotalCount 1)
$payloadContractPassed =
  $manifest.package_id -eq "zero.system.reference" -and
  $manifest.minimum_runtime_major -eq 4 -and
  $manifest.zero_resume -eq $true -and
  $manifest.zero_achievements -eq $true -and
  $manifest.zero_overlay -eq $true -and
  $integrityHeader -eq "# ZERO package integrity v1"

$referenceStatus = if ($payloadContractPassed) { "PASS" } else { "FAIL" }
$referenceExitCode = if ($payloadContractPassed) { 0 } else { 2 }
$referenceOutput = if ($payloadContractPassed) {
  "Reference package manifest and integrity contract are locked."
} else {
  "Reference package contract does not match M1 requirements."
}
$referenceTimestamp = [DateTime]::UtcNow.ToString("o")
$results.Add([pscustomobject]@{
  name = "reference_package_contract"
  category = "reference_package"
  status = $referenceStatus
  exit_code = $referenceExitCode
  started_at_utc = $referenceTimestamp
  ended_at_utc = $referenceTimestamp
  duration_ms = 0
  output = $referenceOutput
})
Write-Host ("[{0}] reference_package_contract" -f $referenceStatus)

$measuredChecks = @($results)
$totalDurationMs = [int64](($measuredChecks | Measure-Object -Property duration_ms -Sum).Sum)
$slowest = $measuredChecks | Sort-Object duration_ms -Descending | Select-Object -First 1
$runtimeBudgetPassed = $totalDurationMs -le $automatedRuntimeBudgetMs
$budgetTimestamp = [DateTime]::UtcNow.ToString("o")
$results.Add([pscustomobject]@{
  name = "automated_runtime_budget"
  category = "performance"
  status = if ($runtimeBudgetPassed) { "PASS" } else { "FAIL" }
  exit_code = if ($runtimeBudgetPassed) { 0 } else { 2 }
  started_at_utc = $budgetTimestamp
  ended_at_utc = $budgetTimestamp
  duration_ms = 0
  output = "Measured acceptance duration: $totalDurationMs ms; budget: $automatedRuntimeBudgetMs ms."
})
Write-Host ("[{0}] automated_runtime_budget ({1} ms / {2} ms)" -f $(if ($runtimeBudgetPassed) { "PASS" } else { "FAIL" }), $totalDurationMs, $automatedRuntimeBudgetMs)

$failedCount = @($results | Where-Object { $_.status -ne "PASS" }).Count
$allAutomatedPassed = $failedCount -eq 0
$commit = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else {
  try { (& git rev-parse HEAD 2>$null).Trim() } catch { "unknown" }
}
$automatedResult = if ($allAutomatedPassed) { "PASS" } else { "FAIL" }

$hardwareRequirements = @(
  [pscustomobject]@{ name = "windows_11_x64_real_machine"; status = "REQUIRED"; reason = "GitHub Actions runs Windows Server and cannot qualify the supported client OS hardware path." },
  [pscustomobject]@{ name = "physical_xinput_controller_traversal"; status = "REQUIRED"; reason = "Deterministic input tests do not prove physical controller enumeration, reconnect behavior, or couch navigation on release hardware." },
  [pscustomobject]@{ name = "real_game_foreground_recovery"; status = "REQUIRED"; reason = "CI does not prove foreground restoration behavior against arbitrary native full-screen games." },
  [pscustomobject]@{ name = "restart_persistence_journey"; status = "REQUIRED"; reason = "Final qualification must verify persisted ZERO state across an actual Windows restart." },
  [pscustomobject]@{ name = "interactive_first_boot"; status = "REQUIRED"; reason = "First Boot controller/display/audio completion remains an interactive hardware acceptance step." }
)

$report = [ordered]@{
  schema = 3
  generated_at_utc = [DateTime]::UtcNow.ToString("o")
  zero_milestone = "M1 / Runtime V4.1 / Production Architecture"
  commit = $commit
  automated_result = $automatedResult
  rc_qualified = $false
  rc_qualification_note = "Automated PASS does not qualify an RC. Real Windows 11 x64 and physical-controller acceptance is still required."
  performance_metrics = [ordered]@{
    total_acceptance_duration_ms = $totalDurationMs
    runtime_budget_ms = $automatedRuntimeBudgetMs
    runtime_budget_passed = $runtimeBudgetPassed
    slowest_gate = if ($slowest) { $slowest.name } else { "none" }
    slowest_gate_duration_ms = if ($slowest) { $slowest.duration_ms } else { 0 }
  }
  binaries = [ordered]@{
    shell = $zeroShell
    acceptance = $zeroAcceptance
    reference_game = $referenceExe
  }
  automated_checks = $results.ToArray()
  physical_qualification_required = $hardwareRequirements
}

$jsonPath = Join-Path $ReportDir "m1-automated-acceptance.json"
$markdownPath = Join-Path $ReportDir "m1-automated-acceptance.md"
$report | ConvertTo-Json -Depth 8 | Set-Content -Path $jsonPath -Encoding UTF8

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# ZERO M1 Automated Acceptance Report")
$lines.Add("")
$lines.Add("- Commit: ``$commit``")
$lines.Add("- Automated result: **$automatedResult**")
$lines.Add("- RC qualified: **NO**")
$lines.Add("- Generated: $($report.generated_at_utc)")
$lines.Add("- Total measured acceptance duration: **$totalDurationMs ms**")
$lines.Add("- Acceptance runtime budget: **$automatedRuntimeBudgetMs ms**")
$lines.Add("- Slowest gate: **$($report.performance_metrics.slowest_gate)** ($($report.performance_metrics.slowest_gate_duration_ms) ms)")
$lines.Add("")
$lines.Add("## Automated gates")
$lines.Add("")
$lines.Add("| Gate | Category | Result | Duration (ms) |")
$lines.Add("| --- | --- | --- | ---: |")
foreach ($item in $results) {
  $lines.Add("| $($item.name) | $($item.category) | $($item.status) | $($item.duration_ms) |")
}
$lines.Add("")
$lines.Add("## Physical qualification still required")
$lines.Add("")
foreach ($item in $hardwareRequirements) {
  $lines.Add("- **$($item.name)** — $($item.reason)")
}
$lines.Add("")
$lines.Add("> Automated PASS is a release-engineering gate, not a substitute for the real Windows 11 x64 + physical-controller RC qualification journey.")
$lines | Set-Content -Path $markdownPath -Encoding UTF8

Write-Host "ZERO M1 automated acceptance report: $jsonPath"
Write-Host "ZERO M1 automated acceptance summary: $markdownPath"
Write-Host ("ZERO M1 automated acceptance: {0}" -f $automatedResult)
Write-Host "ZERO M1 RC qualification: PENDING PHYSICAL ACCEPTANCE"

if (-not $allAutomatedPassed) { exit 2 }
exit 0
