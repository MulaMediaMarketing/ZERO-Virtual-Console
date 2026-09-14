param(
  [string]$SourceDir = (Split-Path -Parent $PSScriptRoot),
  [string]$InstallDir = "$env:LOCALAPPDATA\Programs\ZERO Virtual Console",
  [string]$DataRoot = "$env:LOCALAPPDATA\ZERO",
  [string]$Version = "0.1.0-rc1",
  [string]$StartMenuPath = "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\ZERO Virtual Console.lnk",
  [switch]$SkipRegistration,
  [switch]$SkipShortcut
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Resolve-Binary([string]$Name) {
  $direct = Join-Path $SourceDir $Name
  if (Test-Path $direct -PathType Leaf) { return (Resolve-Path $direct).Path }
  $release = Join-Path $SourceDir ("build\Release\" + $Name)
  if (Test-Path $release -PathType Leaf) { return (Resolve-Path $release).Path }
  return $null
}

function Assert-NonEmptyFile([string]$Path, [string]$Label) {
  if (-not (Test-Path $Path -PathType Leaf)) { throw "$Label was not found." }
  if ((Get-Item $Path).Length -le 0) { throw "$Label is empty." }
}

function Remove-TreeBestEffort([string]$Path) {
  if (-not $Path -or -not (Test-Path $Path)) { return }
  try { Remove-Item $Path -Recurse -Force -ErrorAction Stop } catch { }
}

$exe = Resolve-Binary "ZeroVirtualConsole.exe"
if (-not $exe) { throw "ZeroVirtualConsole.exe was not found." }
Assert-NonEmptyFile $exe "ZeroVirtualConsole.exe"

$acceptance = Resolve-Binary "ZeroAcceptance.exe"
if (-not $acceptance) { throw "ZeroAcceptance.exe was not found. The RC payload is incomplete." }
Assert-NonEmptyFile $acceptance "ZeroAcceptance.exe"

$uninstallSource = Join-Path $PSScriptRoot "uninstall-zero.ps1"
Assert-NonEmptyFile $uninstallSource "uninstall-zero.ps1"

$optionalPayload = @{}
foreach ($name in @("ZeroTestGame.exe", "ZeroReferenceGame.exe")) {
  $resolved = Resolve-Binary $name
  if ($resolved) {
    Assert-NonEmptyFile $resolved $name
    $optionalPayload[$name] = $resolved
  }
}

$installParent = Split-Path -Parent $InstallDir
if (-not $installParent) { throw "ZERO install directory must have a parent directory." }
New-Item -ItemType Directory -Force -Path $installParent | Out-Null

$transactionId = [Guid]::NewGuid().ToString("N")
$stageDir = Join-Path $installParent (".zero-install-stage-" + $transactionId)
$backupDir = Join-Path $installParent (".zero-install-backup-" + $transactionId)
$previousVersion = $null
$oldState = Join-Path $InstallDir "install-state.json"
if (Test-Path $oldState -PathType Leaf) {
  try { $previousVersion = (Get-Content $oldState -Raw | ConvertFrom-Json).version } catch { }
}

try {
  New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
  Copy-Item $exe (Join-Path $stageDir "ZeroVirtualConsole.exe") -Force
  Copy-Item $acceptance (Join-Path $stageDir "ZeroAcceptance.exe") -Force
  foreach ($entry in $optionalPayload.GetEnumerator()) {
    Copy-Item $entry.Value (Join-Path $stageDir $entry.Key) -Force
  }
  Copy-Item $uninstallSource (Join-Path $stageDir "uninstall-zero.ps1") -Force

  foreach ($requiredName in @("ZeroVirtualConsole.exe", "ZeroAcceptance.exe", "uninstall-zero.ps1")) {
    Assert-NonEmptyFile (Join-Path $stageDir $requiredName) $requiredName
  }

  $state = [ordered]@{
    schema = 1
    version = $Version
    previous_version = $previousVersion
    installed_at_utc = [DateTime]::UtcNow.ToString("o")
    install_dir = $InstallDir
    data_root = $DataRoot
  }
  $state | ConvertTo-Json -Depth 3 | Set-Content -Path (Join-Path $stageDir "install-state.json") -Encoding UTF8
  $roundTrip = Get-Content (Join-Path $stageDir "install-state.json") -Raw | ConvertFrom-Json
  if ($roundTrip.schema -ne 1 -or $roundTrip.version -ne $Version) {
    throw "ZERO staged install metadata failed validation."
  }

  if (Test-Path $InstallDir) {
    if (Test-Path $backupDir) { Remove-Item $backupDir -Recurse -Force }
    Move-Item $InstallDir $backupDir
  }

  try {
    Move-Item $stageDir $InstallDir
    foreach ($requiredName in @("ZeroVirtualConsole.exe", "ZeroAcceptance.exe", "uninstall-zero.ps1", "install-state.json")) {
      Assert-NonEmptyFile (Join-Path $InstallDir $requiredName) $requiredName
    }
    $activeState = Get-Content (Join-Path $InstallDir "install-state.json") -Raw | ConvertFrom-Json
    if ($activeState.schema -ne 1 -or $activeState.version -ne $Version) {
      throw "ZERO activated install metadata failed validation."
    }
  } catch {
    Remove-TreeBestEffort $InstallDir
    if (Test-Path $backupDir) { Move-Item $backupDir $InstallDir }
    throw
  }

  New-Item -ItemType Directory -Force -Path $DataRoot | Out-Null
  foreach ($folder in @("Library", "Saves", "Cache", "Temp", "Captures", "Data", "Diagnostics", "Trust")) {
    New-Item -ItemType Directory -Force -Path (Join-Path $DataRoot $folder) | Out-Null
  }

  if (-not $SkipShortcut) {
    $shortcutParent = Split-Path -Parent $StartMenuPath
    if ($shortcutParent) { New-Item -ItemType Directory -Force -Path $shortcutParent | Out-Null }
    $ws = New-Object -ComObject WScript.Shell
    $shortcut = $ws.CreateShortcut($StartMenuPath)
    $shortcut.TargetPath = Join-Path $InstallDir "ZeroVirtualConsole.exe"
    $shortcut.WorkingDirectory = $InstallDir
    $shortcut.Description = "ZERO Virtual Console"
    $shortcut.Save()
  }

  if (-not $SkipRegistration) {
    $uninstallKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\ZERO Virtual Console"
    New-Item -Path $uninstallKey -Force | Out-Null
    New-ItemProperty -Path $uninstallKey -Name DisplayName -Value "ZERO Virtual Console" -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $uninstallKey -Name DisplayVersion -Value $Version -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $uninstallKey -Name Publisher -Value "ZERO" -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $uninstallKey -Name InstallLocation -Value $InstallDir -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $uninstallKey -Name DisplayIcon -Value (Join-Path $InstallDir "ZeroVirtualConsole.exe") -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $uninstallKey -Name UninstallString -Value "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$InstallDir\uninstall-zero.ps1`"" -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $uninstallKey -Name QuietUninstallString -Value "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$InstallDir\uninstall-zero.ps1`"" -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $uninstallKey -Name NoModify -Value 1 -PropertyType DWord -Force | Out-Null
    New-ItemProperty -Path $uninstallKey -Name NoRepair -Value 1 -PropertyType DWord -Force | Out-Null
  }

  Remove-TreeBestEffort $backupDir
} catch {
  Remove-TreeBestEffort $stageDir
  if ((-not (Test-Path $InstallDir)) -and (Test-Path $backupDir)) {
    try { Move-Item $backupDir $InstallDir } catch { }
  }
  throw
} finally {
  Remove-TreeBestEffort $stageDir
}

$mode = if ($previousVersion) { "updated from $previousVersion to $Version" } else { "installed as $Version" }
Write-Host "ZERO Virtual Console $mode at $InstallDir"
Write-Host "Player saves, library packages, settings, and diagnostics remain under $DataRoot and are not replaced during updates."
Write-Host "Acceptance tool installed at $(Join-Path $InstallDir 'ZeroAcceptance.exe')"
