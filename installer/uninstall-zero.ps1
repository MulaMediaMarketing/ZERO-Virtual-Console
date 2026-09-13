param(
  [string]$InstallDir = "$env:LOCALAPPDATA\Programs\ZERO Virtual Console",
  [switch]$RemovePlayerData
)

$ErrorActionPreference = "Stop"
$startMenu = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\ZERO Virtual Console.lnk"
if (Test-Path $startMenu) { Remove-Item $startMenu -Force }

$uninstallKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\ZERO Virtual Console"
if (Test-Path $uninstallKey) { Remove-Item $uninstallKey -Recurse -Force }

if (Test-Path $InstallDir) {
  Get-ChildItem $InstallDir -Force | Where-Object { $_.Name -ne "uninstall-zero.ps1" } | Remove-Item -Recurse -Force
}

if ($RemovePlayerData) {
  $dataRoot = Join-Path $env:LOCALAPPDATA "ZERO"
  if (Test-Path $dataRoot) { Remove-Item $dataRoot -Recurse -Force }
  Write-Host "ZERO Virtual Console and player data removed."
} else {
  Write-Host "ZERO Virtual Console removed. Player saves, Library, Resume data, achievements, and diagnostics were preserved."
}

Write-Host "You may delete $InstallDir after this script exits if the folder remains."
