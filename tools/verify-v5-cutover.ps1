param(
    [string]$SourceRoot = ".",
    [string]$ReportDir = "./build/V5CutoverReports"
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path $SourceRoot).Path
$errors = New-Object System.Collections.Generic.List[string]

function Require-Text([string]$Path, [string]$Pattern, [string]$Message) {
    $full = Join-Path $root $Path
    if (-not (Test-Path $full)) {
        $errors.Add("Missing required file: $Path")
        return
    }
    $text = Get-Content -LiteralPath $full -Raw
    if ($text -notmatch $Pattern) { $errors.Add($Message) }
}

function Forbid-Text([string]$Path, [string]$Pattern, [string]$Message) {
    $full = Join-Path $root $Path
    if (-not (Test-Path $full)) { return }
    $text = Get-Content -LiteralPath $full -Raw
    if ($text -match $Pattern) { $errors.Add($Message) }
}

Require-Text "include/App.h" '#include\s+"v5/ProductionRuntime\.h"' "App must include V5 ProductionRuntime."
Require-Text "include/App.h" 'ProductionRuntime\s+runtime_' "App runtime ownership must be ProductionRuntime."
Forbid-Text "include/App.h" 'RuntimeV[34]' "App must not reference RuntimeV3 or RuntimeV4."

Require-Text "include/App.h" 'v5::ProductionShellIntegration\s+productionShell_' "App must own the V5 production shell integration."
Require-Text "include/App.h" 'AuthoritativePageProjection\s+page_\{productionShell_\}' "Live page state must project from the V5 ShellKernel authority."
Require-Text "include/App.h" 'AuthoritativeNavProjection\s+navIndex_\{productionShell_\}' "Live navigation selection must project from the V5 ShellKernel authority."
Forbid-Text "include/App.h" '(?m)^\s*Page\s+page_\s*\{' "App must not own a raw legacy page state authority."
Forbid-Text "include/App.h" '(?m)^\s*size_t\s+navIndex_\s*\{' "App must not own a raw legacy navigation-index authority."
Forbid-Text "include/v5/ProductionShellIntegration.h" 'NavigateLegacy|ActiveLegacyPage|ToLegacyPage' "Legacy shell bridge APIs must not remain after V5 live-shell cutover."
Require-Text "include/v5/ShellKernel.h" 'MoveTopLevel\s*\(int\s+direction' "ShellKernel must own top-level traversal."
Require-Text "include/v5/ShellKernel.h" 'ActiveTopLevelIndex\s*\(\)\s+const' "ShellKernel must expose its authoritative top-level position."
Require-Text "src/v5/ProductionShellIntegration.cpp" 'return\s+kernel_\.MoveTopLevel\(direction,\s*error\)' "ProductionShellIntegration must delegate top-level traversal to ShellKernel."
Require-Text "include/ProductionUxContract.h" 'ProductionUxNavCount\(\)\s*==\s*12' "Live production navigation contract must expose all twelve V5 permanent destinations."
Require-Text "tools/V5ProductionUxIntegrationAcceptance.cpp" 'ProductionUxNavCount\(\)\s*!=\s*std::size\(kTopLevelPages\)' "Production UX acceptance must prove the live navigation contract matches the V5 top-level shell."
Require-Text "tools/V5ShellKernelAcceptance.cpp" 'MoveTopLevel\(1,\s*error\)' "ShellKernel acceptance must exercise authoritative top-level traversal."

Require-Text "cmake/ZeroV5.cmake" 'src/v5/ProductionRuntime\.cpp' "ProductionRuntime.cpp must be compiled into ZeroVirtualConsole."
Require-Text "cmake/ZeroV5.cmake" 'src/RuntimeV3\.cpp[\s\S]*src/RuntimeV4\.cpp[\s\S]*HEADER_FILE_ONLY' "Legacy runtime implementations must be excluded from the consumer executable."

$productionRuntime = Join-Path $root "src/v5/ProductionRuntime.cpp"
if (-not (Test-Path $productionRuntime)) { $errors.Add("Missing src/v5/ProductionRuntime.cpp") }

$v5Sources = Get-ChildItem -LiteralPath (Join-Path $root "src/v5") -Filter *.cpp -File -ErrorAction SilentlyContinue
foreach ($file in $v5Sources) {
    $text = Get-Content -LiteralPath $file.FullName -Raw
    if ($text -match '#include\s+"RuntimeV[34]\.h"') {
        $errors.Add("V5 source imports legacy runtime authority: $($file.Name)")
    }
}

$legacyHeaders = @("include/RuntimeV3.h", "include/RuntimeV4.h")
foreach ($header in $legacyHeaders) {
    if (-not (Test-Path (Join-Path $root $header))) {
        continue
    }
    $consumerFiles = @(
        "include/App.h",
        "src/App.cpp",
        "src/AppPages.cpp",
        "src/AppAchievements.cpp",
        "src/AppLaunchRecovery.cpp",
        "src/AppSettings.cpp",
        "src/AppUx.cpp"
    )
    $legacyName = [IO.Path]::GetFileNameWithoutExtension($header)
    foreach ($consumer in $consumerFiles) {
        $full = Join-Path $root $consumer
        if (Test-Path $full) {
            $text = Get-Content -LiteralPath $full -Raw
            if ($text -match [regex]::Escape($legacyName)) {
                $errors.Add("Consumer shell still references $legacyName in $consumer")
            }
        }
    }
}

New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
$report = [ordered]@{
    schema = 3
    gate = "zero-v5-final-cutover"
    passed = ($errors.Count -eq 0)
    production_runtime = "src/v5/ProductionRuntime.cpp"
    production_shell = "src/v5/ProductionShellIntegration.cpp"
    shell_authority = "v5-shell-kernel"
    navigation_traversal_authority = "v5-shell-kernel"
    permanent_destinations = 12
    legacy_runtime_authority = "excluded-from-consumer-build"
    errors = @($errors)
}
$report | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $ReportDir "v5-cutover.json") -Encoding UTF8

if ($errors.Count -ne 0) {
    $errors | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host "ZERO V5 final cutover gate: PASS"
