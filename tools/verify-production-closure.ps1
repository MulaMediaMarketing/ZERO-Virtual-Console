param([string]$SourceRoot = ".")
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$violations = New-Object System.Collections.Generic.List[string]
function RequireAbsent([string]$path,[string]$pattern,[string]$message) {
  $text = Get-Content (Join-Path $SourceRoot $path) -Raw
  if ($text -match $pattern) { $violations.Add("$path :: $message") }
}
function RequirePresent([string]$path,[string]$pattern,[string]$message) {
  $text = Get-Content (Join-Path $SourceRoot $path) -Raw
  if ($text -notmatch $pattern) { $violations.Add("$path :: $message") }
}

RequireAbsent "include/App.h" '#include\s+"ResumeStore\.h"' "App may not include legacy ResumeStore"
RequireAbsent "include/App.h" '#include\s+"GameImportService\.h"' "App may not include GameImportService directly"
RequireAbsent "include/App.h" '\bResumeStore\s+[A-Za-z_][A-Za-z0-9_]*\s*[;{]' "App may not own a ResumeStore"
RequireAbsent "include/App.h" '\bGameImportService\s+[A-Za-z_][A-Za-z0-9_]*\s*[;{]' "App may not own GameImportService"
RequirePresent "include/App.h" 'v5::ImportCoordinator\s+importer_' "App import must route through V5 ImportCoordinator"
RequirePresent "include/App.h" 'AuthoritativeResumeProjection\s+resumeStore_\{runtime_\}' "App resume reads must project through ProductionRuntime"
RequirePresent "include/App.h" 'v5::ProductionShellIntegration\s+productionShell_' "ShellKernel integration must remain authoritative"
RequireAbsent "include/App.h" '(?m)^\s*Page\s+page_\s*\{' "Raw App page authority is forbidden"
RequireAbsent "include/App.h" '(?m)^\s*size_t\s+navIndex_\s*\{' "Raw App nav authority is forbidden"
RequirePresent "cmake/ZeroV5.cmake" 'src/v5/ImportCoordinator\.cpp' "V5 ImportCoordinator must be in production target"
RequireAbsent "src/v5/ProductionRuntime.cpp" 'return\s+resumeStore_\.Load\(' "ProductionRuntime may not fall back to legacy resume persistence"
RequireAbsent "src/v5/ProductionRuntime.cpp" 'stateStore_\.RecordSession\(' "ProductionRuntime may not mirror canonical sessions to legacy storage"

if ($violations.Count -gt 0) {
  Write-Host "ZERO production closure gate: FAIL"
  foreach ($item in $violations) { Write-Host "PRODUCTION CLOSURE VIOLATION: $item" }
  exit 2
}
Write-Host "ZERO production closure gate: PASS"
exit 0
