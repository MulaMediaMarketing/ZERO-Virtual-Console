param(
  [string]$InstallDir = "$env:LOCALAPPDATA\Programs\ZERO Virtual Console",
  [string]$DataRoot = "$env:LOCALAPPDATA\ZERO",
  [string]$StartMenuPath = "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\ZERO Virtual Console.lnk",
  [switch]$RemovePlayerData,
  [switch]$SkipRegistration,
  [switch]$SkipShortcut
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Remove-Tree([string]$Path, [string]$Label) {
  if (-not (Test-Path $Path)) { return }
  Remove-Item $Path -Recurse -Force -ErrorAction Stop
  if (Test-Path $Path) { throw "ZERO could not remove $Label at '$Path'." }
}

if (-not $SkipShortcut -and (Test-Path $StartMenuPath -PathType Leaf)) {
  Remove-Item $StartMenuPath -Force
}

if (-not $SkipRegistration) {
  $uninstallKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\ZERO Virtual Console"
  if (Test-Path $uninstallKey) { Remove-Item $uninstallKey -Recurse -Force }
}

# Player-owned and platform state live outside the program directory. Never touch
# DataRoot unless the caller explicitly supplies -RemovePlayerData.
if ($RemovePlayerData) {
  Remove-Tree $DataRoot "ZERO player data"
}

if (Test-Path $InstallDir) {
  $self = $MyInvocation.MyCommand.Path
  $selfInsideInstall = $self -and ([IO.Path]::GetFullPath($self)).StartsWith([IO.Path]::GetFullPath($InstallDir), [StringComparison]::OrdinalIgnoreCase)
  if ($selfInsideInstall) {
    Get-ChildItem $InstallDir -Force | Where-Object { $_.FullName -ne $self } | Remove-Item -Recurse -Force
  } else {
    Remove-Tree $InstallDir "ZERO program files"
  }
}

if ($RemovePlayerData) {
  Write-Host "ZERO Virtual Console and explicitly requested player data removed."
} else {
  Write-Host "ZERO Virtual Console removed. Player saves, Library, Resume data, achievements, settings, trust data, captures, and diagnostics were preserved."
}

if (Test-Path $InstallDir) {
  Write-Host "The installer program folder contains only the running uninstall script and may be deleted after this script exits: $InstallDir"
}
