$ErrorActionPreference = "Stop"

$root = Join-Path $env:LOCALAPPDATA "ZERO"
$library = Join-Path $root "Library"
$runtime = Join-Path $root "Runtime"
$saves = Join-Path $root "Saves"
$cache = Join-Path $root "Cache"
$temp = Join-Path $root "Temp"

@($root, $library, $runtime, $saves, $cache, $temp) | ForEach-Object {
    New-Item -ItemType Directory -Force -Path $_ | Out-Null
}

Write-Host "ZERO development data root: $root"
Write-Host "Game library: $library"
Write-Host ""
Write-Host "To register a game, copy a folder containing zero.manifest.json and its native Windows .exe into the Library directory."
