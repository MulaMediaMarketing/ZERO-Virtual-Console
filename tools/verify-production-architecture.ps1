param(
  [string]$SourceRoot = ".",
  [string]$BuildRoot = "./build",
  [string]$ReportDir = "./build/ArchitectureReports"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$sourceRootResolved = (Resolve-Path $SourceRoot).Path
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null

$sourceExtensions = @(".cpp", ".h", ".hpp", ".ps1", ".cmake")
$sourceFiles = @(Get-ChildItem -Path $SourceRoot -Recurse -File | Where-Object {
  $ext = $_.Extension.ToLowerInvariant()
  $isSource = $sourceExtensions -contains $ext -or $_.Name -eq "CMakeLists.txt"
  $isIgnored = $_.FullName -match "[\\/](build|\.git)[\\/]"
  $isSource -and -not $isIgnored
})

$violations = New-Object System.Collections.Generic.List[object]
$fileMetrics = New-Object System.Collections.Generic.List[object]
$totalLines = 0
$maxLines = 0
$maxFile = ""

foreach ($file in $sourceFiles) {
  $relative = [IO.Path]::GetRelativePath($sourceRootResolved, $file.FullName).Replace('\\','/')
  $lines = @(Get-Content $file.FullName).Count
  $totalLines += $lines
  if ($lines -gt $maxLines) { $maxLines = $lines; $maxFile = $relative }
  $fileMetrics.Add([pscustomobject]@{ path = $relative; lines = $lines; bytes = $file.Length })
  if ($lines -gt 1000) {
    $violations.Add([pscustomobject]@{ severity = "error"; rule = "source_file_max_1000_lines"; path = $relative; detail = "$lines lines" })
  }
}

# Debt markers are enforced in executable/configuration source, not prose documentation,
# where the words may legitimately appear while describing the policy itself.
$scanFiles = @(Get-ChildItem -Path $SourceRoot -Recurse -File | Where-Object {
  $_.Extension.ToLowerInvariant() -in @(".cpp", ".h", ".hpp", ".ps1", ".yml", ".yaml") -and
  $_.FullName -notmatch "[\\/](build|\.git)[\\/]"
})

$debtPattern = '(?i)\b(TODO|FIXME|HACK)\b|\bprototype[- ]only\b|\bmock data\b|\bplaceholder data\b'
foreach ($file in $scanFiles) {
  $matches = Select-String -Path $file.FullName -Pattern $debtPattern -AllMatches
  foreach ($match in $matches) {
    $relative = [IO.Path]::GetRelativePath($sourceRootResolved, $file.FullName).Replace('\\','/')
    $violations.Add([pscustomobject]@{ severity = "error"; rule = "explicit_technical_debt_marker"; path = $relative; detail = "line $($match.LineNumber)" })
  }
}

$readme = Get-Content (Join-Path $SourceRoot "README.md") -Raw
$architecture = Get-Content (Join-Path $SourceRoot "docs/ARCHITECTURE.md") -Raw
$trustDoc = Get-Content (Join-Path $SourceRoot "docs/PACKAGE_TRUST.md") -Raw
$productionStandard = Get-Content (Join-Path $SourceRoot "docs/PRODUCTION_ARCHITECTURE_STANDARD.md") -Raw

if ($readme -match 'Runtime V2 architecture' -or $architecture -match 'RuntimeSession V2') {
  $violations.Add([pscustomobject]@{ severity = "error"; rule = "current_docs_runtime_version"; path = "README.md/docs/ARCHITECTURE.md"; detail = "Current documentation must describe Runtime V4.1." })
}
if ($trustDoc -notmatch 'ecdsa-p256-sha256') {
  $violations.Add([pscustomobject]@{ severity = "error"; rule = "publisher_crypto_documentation"; path = "docs/PACKAGE_TRUST.md"; detail = "Package trust documentation must match ECDSA P-256/SHA-256 implementation." })
}
if ($productionStandard -notmatch 'Single authority per concern' -or $productionStandard -notmatch 'Measurable release benchmarks') {
  $violations.Add([pscustomobject]@{ severity = "error"; rule = "production_standard_present"; path = "docs/PRODUCTION_ARCHITECTURE_STANDARD.md"; detail = "Production architecture standard is incomplete." })
}

$appPath = Join-Path $SourceRoot "src/App.cpp"
if (Test-Path $appPath) {
  $app = Get-Content $appPath -Raw
  if ($app -match 'App::Page pageForNavIndex') {
    $violations.Add([pscustomobject]@{ severity = "error"; rule = "single_navigation_authority"; path = "src/App.cpp"; detail = "Shell navigation must use ProductionUxContract rather than duplicate page mapping." })
  }
  if ($app -match 'navIndex_\s*=\s*[34]\s*;') {
    $violations.Add([pscustomobject]@{ severity = "error"; rule = "no_magic_navigation_indexes"; path = "src/App.cpp"; detail = "Top-level navigation indexes must come from ProductionUxContract." })
  }
}

$acceptanceSources = @(Get-ChildItem -Path (Join-Path $SourceRoot "tools") -Filter "*Acceptance.cpp" -File -ErrorAction SilentlyContinue)
$acceptanceBinaries = @(Get-ChildItem -Path (Join-Path $BuildRoot "Release") -Filter "Zero*Acceptance.exe" -File -ErrorAction SilentlyContinue)
if ($acceptanceSources.Count -lt 10) {
  $violations.Add([pscustomobject]@{ severity = "error"; rule = "minimum_acceptance_contracts"; path = "tools"; detail = "Expected at least 10 dedicated acceptance sources; found $($acceptanceSources.Count)." })
}
if ($acceptanceBinaries.Count -lt $acceptanceSources.Count) {
  $violations.Add([pscustomobject]@{ severity = "error"; rule = "acceptance_binary_coverage"; path = "$BuildRoot/Release"; detail = "Built acceptance binaries ($($acceptanceBinaries.Count)) are fewer than acceptance sources ($($acceptanceSources.Count))." })
}

$binaryMetrics = New-Object System.Collections.Generic.List[object]
foreach ($binary in $acceptanceBinaries) {
  $binaryMetrics.Add([pscustomobject]@{
    name = $binary.Name
    bytes = $binary.Length
    sha256 = (Get-FileHash -Algorithm SHA256 $binary.FullName).Hash.ToLowerInvariant()
  })
}

$shell = Join-Path $BuildRoot "Release/ZeroVirtualConsole.exe"
$shellMetric = $null
if (-not (Test-Path $shell -PathType Leaf)) {
  $violations.Add([pscustomobject]@{ severity = "error"; rule = "release_shell_present"; path = $shell; detail = "Release shell binary missing." })
} else {
  $shellFile = Get-Item $shell
  $shellMetric = [pscustomobject]@{
    bytes = $shellFile.Length
    sha256 = (Get-FileHash -Algorithm SHA256 $shellFile.FullName).Hash.ToLowerInvariant()
  }
  if ($shellFile.Length -ge 64MB) {
    $violations.Add([pscustomobject]@{ severity = "error"; rule = "shell_binary_size_budget"; path = $shell; detail = "Shell exceeds 64 MiB production budget." })
  }
}

$commit = if ($env:GITHUB_SHA) { $env:GITHUB_SHA } else {
  try { (& git rev-parse HEAD 2>$null).Trim() } catch { "unknown" }
}
$failed = @($violations | Where-Object { $_.severity -eq "error" }).Count
$status = if ($failed -eq 0) { "PASS" } else { "FAIL" }

$report = [ordered]@{
  schema = 2
  generated_at_utc = [DateTime]::UtcNow.ToString("o")
  commit = $commit
  result = $status
  policy = [ordered]@{
    max_source_lines = 1000
    minimum_acceptance_contracts = 10
    shell_binary_max_bytes = 67108864
    current_runtime = "V4.1"
    publisher_signature_algorithm = "ecdsa-p256-sha256"
  }
  source_metrics = [ordered]@{
    files = $sourceFiles.Count
    total_lines = $totalLines
    max_file_lines = $maxLines
    max_file = $maxFile
    files_detail = $fileMetrics.ToArray()
  }
  shell_metric = $shellMetric
  acceptance_metrics = [ordered]@{
    source_contracts = $acceptanceSources.Count
    built_binaries = $acceptanceBinaries.Count
    binaries = $binaryMetrics.ToArray()
  }
  violations = $violations.ToArray()
}

$jsonPath = Join-Path $ReportDir "production-architecture.json"
$mdPath = Join-Path $ReportDir "production-architecture.md"
$report | ConvertTo-Json -Depth 10 | Set-Content -Path $jsonPath -Encoding UTF8

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# ZERO Production Architecture Report")
$lines.Add("")
$lines.Add("- Commit: ``$commit``")
$lines.Add("- Result: **$status**")
$lines.Add("- Source files: $($sourceFiles.Count)")
$lines.Add("- Source lines: $totalLines")
$lines.Add("- Largest source file: ``$maxFile`` ($maxLines lines)")
$lines.Add("- Acceptance contracts: $($acceptanceSources.Count)")
$lines.Add("- Built acceptance binaries: $($acceptanceBinaries.Count)")
if ($shellMetric) { $lines.Add("- Shell bytes: $($shellMetric.bytes)") }
$lines.Add("")
$lines.Add("## Policy violations")
$lines.Add("")
if ($violations.Count -eq 0) {
  $lines.Add("None.")
} else {
  foreach ($item in $violations) { $lines.Add("- **$($item.rule)** — ``$($item.path)`` — $($item.detail)") }
}
$lines | Set-Content -Path $mdPath -Encoding UTF8

Write-Host "ZERO production architecture result: $status"
Write-Host "ZERO production architecture report: $jsonPath"
if ($failed -gt 0) { exit 2 }
exit 0
