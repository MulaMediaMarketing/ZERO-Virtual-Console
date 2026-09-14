param(
  [string]$BuildRoot = "./build"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$sourceRoot = $repoRoot
$installScript = Join-Path $repoRoot "installer\install-zero.ps1"
$uninstallScript = Join-Path $repoRoot "installer\uninstall-zero.ps1"
$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("zero-installer-v2-" + [Guid]::NewGuid().ToString("N"))
$installDir = Join-Path $tempRoot "Program\ZERO Virtual Console"
$dataRoot = Join-Path $tempRoot "Data\ZERO"
$shortcut = Join-Path $tempRoot "ZERO Virtual Console.lnk"
$installParent = Split-Path -Parent $installDir

function Check([bool]$Condition, [string]$Name) {
  if (-not $Condition) { throw "[FAIL] $Name" }
  Write-Host "[PASS] $Name"
}

try {
  New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null

  & $installScript -SourceDir $sourceRoot -InstallDir $installDir -DataRoot $dataRoot -Version "0.1.0-test1" -StartMenuPath $shortcut -SkipRegistration -SkipShortcut
  Check (Test-Path (Join-Path $installDir "ZeroVirtualConsole.exe") -PathType Leaf) "initial_install_program"
  Check (Test-Path (Join-Path $installDir "ZeroAcceptance.exe") -PathType Leaf) "initial_install_acceptance"
  Check (Test-Path (Join-Path $installDir "install-state.json") -PathType Leaf) "initial_install_state"
  $state1 = Get-Content (Join-Path $installDir "install-state.json") -Raw | ConvertFrom-Json
  Check ($state1.version -eq "0.1.0-test1") "initial_version_recorded"

  $save = Join-Path $dataRoot "Saves\zero.test\slot1.sav"
  New-Item -ItemType Directory -Force -Path (Split-Path -Parent $save) | Out-Null
  Set-Content -Path $save -Value "player-save-sentinel" -NoNewline
  $settingsSentinel = Join-Path $dataRoot "settings-sentinel.txt"
  Set-Content -Path $settingsSentinel -Value "settings-preserved" -NoNewline

  Set-Content -Path (Join-Path $installDir "stale-program-file.tmp") -Value "must disappear on update" -NoNewline
  & $installScript -SourceDir $sourceRoot -InstallDir $installDir -DataRoot $dataRoot -Version "0.1.0-test2" -StartMenuPath $shortcut -SkipRegistration -SkipShortcut

  $state2 = Get-Content (Join-Path $installDir "install-state.json") -Raw | ConvertFrom-Json
  Check ($state2.version -eq "0.1.0-test2") "update_version_recorded"
  Check ($state2.previous_version -eq "0.1.0-test1") "previous_version_recorded"
  Check (-not (Test-Path (Join-Path $installDir "stale-program-file.tmp"))) "update_replaces_program_directory"
  Check ((Get-Content $save -Raw) -eq "player-save-sentinel") "update_preserves_saves"
  Check ((Get-Content $settingsSentinel -Raw) -eq "settings-preserved") "update_preserves_platform_data"

  $transactionDebris = @(Get-ChildItem $installParent -Force -ErrorAction SilentlyContinue | Where-Object { $_.Name -like ".zero-install-*" })
  Check ($transactionDebris.Count -eq 0) "update_cleans_transaction_directories"

  & $uninstallScript -InstallDir $installDir -DataRoot $dataRoot -StartMenuPath $shortcut -SkipRegistration -SkipShortcut
  Check (-not (Test-Path $installDir)) "uninstall_removes_program_files"
  Check ((Get-Content $save -Raw) -eq "player-save-sentinel") "uninstall_preserves_saves_by_default"
  Check ((Get-Content $settingsSentinel -Raw) -eq "settings-preserved") "uninstall_preserves_platform_data_by_default"

  & $installScript -SourceDir $sourceRoot -InstallDir $installDir -DataRoot $dataRoot -Version "0.1.0-test3" -StartMenuPath $shortcut -SkipRegistration -SkipShortcut
  Check ((Get-Content $save -Raw) -eq "player-save-sentinel") "reinstall_reuses_preserved_data"

  & $uninstallScript -InstallDir $installDir -DataRoot $dataRoot -StartMenuPath $shortcut -RemovePlayerData -SkipRegistration -SkipShortcut
  Check (-not (Test-Path $installDir)) "explicit_full_uninstall_removes_program_files"
  Check (-not (Test-Path $dataRoot)) "explicit_full_uninstall_removes_player_data"

  Write-Host "ZERO Installer / Update / Uninstall V2 acceptance: PASS"
} finally {
  if (Test-Path $tempRoot) { Remove-Item $tempRoot -Recurse -Force -ErrorAction SilentlyContinue }
}
