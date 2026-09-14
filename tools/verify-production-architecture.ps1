param(
  [string]$SourceRoot = ".",
  [string]$BuildRoot = "./build",
  [string]$ReportDir = "./build/ArchitectureReports"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$root = (Resolve-Path $SourceRoot).Path
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null
$violations = New-Object System.Collections.Generic.List[object]
function Violate([string]$rule,[string]$path,[string]$detail) {
  $violations.Add([pscustomobject]@{severity="error";rule=$rule;path=$path;detail=$detail})
}

$sourceFiles = @(Get-ChildItem $SourceRoot -Recurse -File | Where-Object {
  ($_.Extension.ToLowerInvariant() -in @(".cpp",".h",".hpp",".ps1",".cmake") -or $_.Name -eq "CMakeLists.txt") -and
  $_.FullName -notmatch "[\\/](build|\.git)[\\/]"
})
$fileMetrics = New-Object System.Collections.Generic.List[object]
$totalLines=0; $maxLines=0; $maxFile=""
foreach($file in $sourceFiles) {
  $relative=[IO.Path]::GetRelativePath($root,$file.FullName).Replace('\\','/')
  $lines=@(Get-Content $file.FullName).Count
  $totalLines += $lines
  if($lines -gt $maxLines){$maxLines=$lines;$maxFile=$relative}
  $fileMetrics.Add([pscustomobject]@{path=$relative;lines=$lines;bytes=$file.Length})
  if($lines -gt 1000){Violate "source_file_max_1000_lines" $relative "$lines lines"}
}

$scanFiles=@(Get-ChildItem $SourceRoot -Recurse -File | Where-Object {
  $_.Extension.ToLowerInvariant() -in @(".cpp",".h",".hpp",".ps1",".yml",".yaml") -and $_.FullName -notmatch "[\\/](build|\.git)[\\/]"
})
$debtPattern='(?i)\b(TODO|FIXME|HACK)\b|\bprototype[- ]only\b|\bmock data\b|\bplaceholder data\b'
foreach($file in $scanFiles){
  $relative=[IO.Path]::GetRelativePath($root,$file.FullName).Replace('\\','/')
  if($relative -eq "tools/verify-production-architecture.ps1"){continue}
  foreach($match in @(Select-String -Path $file.FullName -Pattern $debtPattern -AllMatches)){
    Violate "explicit_technical_debt_marker" $relative "line $($match.LineNumber)"
  }
}

$cmake=Get-Content (Join-Path $SourceRoot "CMakeLists.txt") -Raw
foreach($flag in @('/W4','/WX','/permissive-','/utf-8')){
  if($cmake -notmatch [regex]::Escape($flag)){Violate "production_compiler_policy" "CMakeLists.txt" "Missing MSVC flag $flag"}
}
$readme=Get-Content (Join-Path $SourceRoot "README.md") -Raw
$architecture=Get-Content (Join-Path $SourceRoot "docs/ARCHITECTURE.md") -Raw
$trustDoc=Get-Content (Join-Path $SourceRoot "docs/PACKAGE_TRUST.md") -Raw
$standard=Get-Content (Join-Path $SourceRoot "docs/PRODUCTION_ARCHITECTURE_STANDARD.md") -Raw
if($readme -match 'Runtime V2 architecture' -or $architecture -match 'RuntimeSession V2'){Violate "current_docs_runtime_version" "README.md/docs/ARCHITECTURE.md" "Current docs must describe Runtime V4.1."}
if($trustDoc -notmatch 'ecdsa-p256-sha256'){Violate "publisher_crypto_documentation" "docs/PACKAGE_TRUST.md" "Docs must match ECDSA P-256/SHA-256."}
if($standard -notmatch 'Single authority per concern' -or $standard -notmatch 'Measurable release benchmarks'){Violate "production_standard_present" "docs/PRODUCTION_ARCHITECTURE_STANDARD.md" "Production standard incomplete."}

$requiredIncludes=@{
  "src/GameRegistry.cpp"='PackageManifestParser.h'
  "src/GameImportService.cpp"='PackageManifestParser.h'
  "src/PackageTrust.cpp"='StrictJson.h'
  "src/CngPublisherTrustProvider.cpp"='StrictJson.h'
}
foreach($pair in $requiredIncludes.GetEnumerator()){
  $text=Get-Content (Join-Path $SourceRoot $pair.Key) -Raw
  if($text -notmatch [regex]::Escape($pair.Value)){Violate "strict_package_metadata_parsing" $pair.Key "Must use $($pair.Value)."}
}
$registryText=Get-Content (Join-Path $SourceRoot "src/GameRegistry.cpp") -Raw
$importText=Get-Content (Join-Path $SourceRoot "src/GameImportService.cpp") -Raw
if($registryText -match 'jsonString\s*\(' -or $importText -match 'jsonString\s*\('){Violate "no_handwritten_manifest_json" "src/GameRegistry.cpp/src/GameImportService.cpp" "Package manifests must use the shared typed parser."}

$app=Get-Content (Join-Path $SourceRoot "src/App.cpp") -Raw
$pages=Get-Content (Join-Path $SourceRoot "src/AppPages.cpp") -Raw
$appHeader=Get-Content (Join-Path $SourceRoot "include/App.h") -Raw
if($app -match 'App::Page pageForNavIndex' -or $app -match 'navIndex_\s*=\s*[34]\s*;'){Violate "single_navigation_authority" "src/App.cpp" "Use ProductionUxContract only."}
if($pages -match 'const\s+wchar_t\*\s+nav\s*\['){Violate "single_navigation_label_authority" "src/AppPages.cpp" "Use ProductionUxContract labels."}
foreach($legacy in @("selectedGame_","selectedCapture_","captureScroll_","selectedSetting_","captureViewerVisible_","captureDeleteConfirm_")){
  if($appHeader -match [regex]::Escape($legacy)){Violate "single_experience_state_authority" "include/App.h" "Legacy state $legacy is forbidden."}
}

$main=Get-Content (Join-Path $SourceRoot "src/main.cpp") -Raw
if($main -notmatch 'DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2'){Violate "per_monitor_dpi_awareness" "src/main.cpp" "Shell must opt into Per-Monitor V2 DPI awareness."}

$pathFiles=@(Get-ChildItem $SourceRoot -Recurse -File | Where-Object {$_.Extension.ToLowerInvariant() -in @(".cpp",".h",".hpp") -and $_.FullName -notmatch "[\\/](build|\.git)[\\/]"})
foreach($file in $pathFiles){
  $relative=[IO.Path]::GetRelativePath($root,$file.FullName).Replace('\\','/')
  if($relative -eq "include/PlatformPaths.h"){continue}
  if(Select-String $file.FullName -Pattern 'SHGetKnownFolderPath\s*\(' -Quiet){Violate "single_platform_path_authority" $relative "Use PlatformPaths."}
}

$acceptanceSources=@(Get-ChildItem (Join-Path $SourceRoot "tools") -Filter "*Acceptance.cpp" -File -ErrorAction SilentlyContinue)
$acceptanceBinaries=@(Get-ChildItem (Join-Path $BuildRoot "Release") -Filter "Zero*Acceptance.exe" -File -ErrorAction SilentlyContinue)
$minimumContracts=14
if($acceptanceSources.Count -lt $minimumContracts){Violate "minimum_acceptance_contracts" "tools" "Expected >=$minimumContracts; found $($acceptanceSources.Count)."}
if($acceptanceBinaries.Count -lt $acceptanceSources.Count){Violate "acceptance_binary_coverage" "$BuildRoot/Release" "Built $($acceptanceBinaries.Count), sources $($acceptanceSources.Count)."}
foreach($required in @("StrictJsonAcceptance.cpp","PlatformDatabaseAcceptance.cpp","FaultInjectionAcceptance.cpp","ProductionBenchmarkAcceptance.cpp")){
  if(-not (Test-Path (Join-Path $SourceRoot "tools/$required"))){Violate "required_production_acceptance" "tools/$required" "Required production acceptance missing."}
}

$binaryMetrics=New-Object System.Collections.Generic.List[object]
foreach($binary in $acceptanceBinaries){$binaryMetrics.Add([pscustomobject]@{name=$binary.Name;bytes=$binary.Length;sha256=(Get-FileHash -Algorithm SHA256 $binary.FullName).Hash.ToLowerInvariant()})}
$shell=Join-Path $BuildRoot "Release/ZeroVirtualConsole.exe"; $shellMetric=$null
if(-not(Test-Path $shell -PathType Leaf)){Violate "release_shell_present" $shell "Release shell missing."}else{
  $shellFile=Get-Item $shell; $shellMetric=[pscustomobject]@{bytes=$shellFile.Length;sha256=(Get-FileHash -Algorithm SHA256 $shellFile.FullName).Hash.ToLowerInvariant()}
  if($shellFile.Length -ge 64MB){Violate "shell_binary_size_budget" $shell "Shell exceeds 64 MiB."}
}

$commit=if($env:GITHUB_SHA){$env:GITHUB_SHA}else{try{(& git rev-parse HEAD 2>$null).Trim()}catch{"unknown"}}
$failed=$violations.Count; $status=if($failed -eq 0){"PASS"}else{"FAIL"}
$report=[ordered]@{
  schema=4;generated_at_utc=[DateTime]::UtcNow.ToString("o");commit=$commit;result=$status
  policy=[ordered]@{max_source_lines=1000;minimum_acceptance_contracts=$minimumContracts;shell_binary_max_bytes=67108864;current_runtime="V4.1";publisher_signature_algorithm="ecdsa-p256-sha256";navigation_authority="ProductionUxContract";platform_path_authority="PlatformPaths";compiler_warnings="fatal";package_json_parser="StrictJson/PackageManifestParser"}
  source_metrics=[ordered]@{files=$sourceFiles.Count;total_lines=$totalLines;max_file_lines=$maxLines;max_file=$maxFile;files_detail=$fileMetrics.ToArray()}
  shell_metric=$shellMetric
  acceptance_metrics=[ordered]@{source_contracts=$acceptanceSources.Count;built_binaries=$acceptanceBinaries.Count;binaries=$binaryMetrics.ToArray()}
  violations=$violations.ToArray()
}
$jsonPath=Join-Path $ReportDir "production-architecture.json";$mdPath=Join-Path $ReportDir "production-architecture.md"
$report|ConvertTo-Json -Depth 10|Set-Content $jsonPath -Encoding UTF8
$lines=New-Object System.Collections.Generic.List[string]
$lines.Add("# ZERO Production Architecture Report");$lines.Add("");$lines.Add("- Commit: ``$commit``");$lines.Add("- Result: **$status**");$lines.Add("- Source files: $($sourceFiles.Count)");$lines.Add("- Source lines: $totalLines");$lines.Add("- Largest source file: ``$maxFile`` ($maxLines lines)");$lines.Add("- Acceptance contracts: $($acceptanceSources.Count)");$lines.Add("- Built acceptance binaries: $($acceptanceBinaries.Count)");$lines.Add("");$lines.Add("## Policy violations")
if($violations.Count -eq 0){$lines.Add("None.")}else{foreach($item in $violations){$lines.Add("- **$($item.rule)** — ``$($item.path)`` — $($item.detail)")}}
$lines|Set-Content $mdPath -Encoding UTF8
Write-Host "ZERO production architecture result: $status"
Write-Host "ZERO production architecture report: $jsonPath"
if($violations.Count -gt 0){
  foreach($item in $violations){Write-Host ("ARCHITECTURE VIOLATION: {0} | {1} | {2}" -f $item.rule,$item.path,$item.detail)}
}
if($failed -gt 0){exit 2}
exit 0
