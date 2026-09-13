param(
  [string]$SourceDir = (Split-Path -Parent $PSScriptRoot),
  [string]$InstallDir = "$env:LOCALAPPDATA\Programs\ZERO Virtual Console"
)

$ErrorActionPreference = "Stop"

function Resolve-Binary([string]$Name) {
  $direct = Join-Path $SourceDir $Name
  if (Test-Path $direct) { return $direct }
  $release = Join-Path $SourceDir ("build\Release\" + $Name)
  if (Test-Path $release) { return $release }
  return $null
}

$exe = Resolve-Binary "ZeroVirtualConsole.exe"
if (-not $exe) { throw "ZeroVirtualConsole.exe was not found." }

$acceptance = Resolve-Binary "ZeroAcceptance.exe"
if (-not $acceptance) { throw "ZeroAcceptance.exe was not found. The RC payload is incomplete." }

New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
Copy-Item $exe (Join-Path $InstallDir "ZeroVirtualConsole.exe") -Force
Copy-Item $acceptance (Join-Path $InstallDir "ZeroAcceptance.exe") -Force

$testGame = Resolve-Binary "ZeroTestGame.exe"
if ($testGame) { Copy-Item $testGame (Join-Path $InstallDir "ZeroTestGame.exe") -Force }

$referenceGame = Resolve-Binary "ZeroReferenceGame.exe"
if ($referenceGame) { Copy-Item $referenceGame (Join-Path $InstallDir "ZeroReferenceGame.exe") -Force }

$dataRoot = Join-Path $env:LOCALAPPDATA "ZERO"
New-Item -ItemType Directory -Force -Path $dataRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $dataRoot "Library") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $dataRoot "Saves") | Out-Null

$startMenu = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\ZERO Virtual Console.lnk"
$ws = New-Object -ComObject WScript.Shell
$shortcut = $ws.CreateShortcut($startMenu)
$shortcut.TargetPath = Join-Path $InstallDir "ZeroVirtualConsole.exe"
$shortcut.WorkingDirectory = $InstallDir
$shortcut.Description = "ZERO Virtual Console"
$shortcut.Save()

$uninstallKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\ZERO Virtual Console"
New-Item -Path $uninstallKey -Force | Out-Null
New-ItemProperty -Path $uninstallKey -Name DisplayName -Value "ZERO Virtual Console" -PropertyType String -Force | Out-Null
New-ItemProperty -Path $uninstallKey -Name DisplayVersion -Value "0.1.0-rc1" -PropertyType String -Force | Out-Null
New-ItemProperty -Path $uninstallKey -Name Publisher -Value "ZERO" -PropertyType String -Force | Out-Null
New-ItemProperty -Path $uninstallKey -Name InstallLocation -Value $InstallDir -PropertyType String -Force | Out-Null
New-ItemProperty -Path $uninstallKey -Name UninstallString -Value "powershell.exe -ExecutionPolicy Bypass -File `"$InstallDir\uninstall-zero.ps1`"" -PropertyType String -Force | Out-Null

Copy-Item (Join-Path $PSScriptRoot "uninstall-zero.ps1") (Join-Path $InstallDir "uninstall-zero.ps1") -Force
Write-Host "ZERO Virtual Console installed to $InstallDir"
Write-Host "Player saves and library data live under $dataRoot"
Write-Host "Acceptance tool installed at $(Join-Path $InstallDir 'ZeroAcceptance.exe')"
