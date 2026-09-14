param(
  [string]$SourceRoot = ".",
  [string]$ReportDir = "./build/DependencyReports"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
New-Item -ItemType Directory -Force -Path $ReportDir | Out-Null

$allowedSystemLibraries = @(
  "advapi32","bcrypt","d2d1","dbghelp","dwrite","ole32","shell32","shlwapi",
  "windowscodecs","winsqlite3","xinput9_1_0"
)
$internalTargets = @("ZeroSdk")
$cmake = Get-Content (Join-Path $SourceRoot "CMakeLists.txt") -Raw
$linkMatches = [regex]::Matches($cmake, 'target_link_libraries\s*\([^\)]*\)', [Text.RegularExpressions.RegexOptions]::IgnoreCase)
$linked = New-Object System.Collections.Generic.HashSet[string]([StringComparer]::OrdinalIgnoreCase)
foreach($match in $linkMatches) {
  $body = $match.Value -replace '^target_link_libraries\s*\(',' ' -replace '\)$',' '
  foreach($token in ($body -split '\s+')) {
    $clean = $token.Trim()
    if(-not $clean -or $clean -in @("PRIVATE","PUBLIC","INTERFACE")){continue}
    if($clean -match '^Zero[A-Za-z0-9_]+$' -or $clean -match '^[A-Za-z0-9_]+$') {
      if($clean -notmatch '^Zero(VirtualConsole|TestGame|ReferenceGame|Acceptance|IpcPersistenceAcceptance|PackageIntegrityAcceptance|PackageTrustAcceptance|PackageRepairAcceptance|StrictJsonAcceptance|PlatformDatabaseAcceptance|FaultInjectionAcceptance|ProductionBenchmarkAcceptance|InputAcceptance|ShellUxAcceptance|ProductionUxAcceptance|FriendsExperienceAcceptance|CapturesExperienceAcceptance|CoreShellExperienceAcceptance|StoreSettingsFirstBootAcceptance|FirstBootPersistenceAcceptance|QualificationProbe)$') {
        [void]$linked.Add($clean)
      }
    }
  }
}

$violations = New-Object System.Collections.Generic.List[string]
foreach($library in $linked) {
  if($library -notin $allowedSystemLibraries -and $library -notin $internalTargets) {
    $violations.Add("Unapproved linked dependency: $library")
  }
}

$binaryExtensions = @('.exe','.dll','.lib','.sys','.msi','.msix','.cab','.ocx')
$vendored = @(Get-ChildItem $SourceRoot -Recurse -File | Where-Object {
  $_.FullName -notmatch '[\\/](build|\.git)[\\/]' -and $binaryExtensions -contains $_.Extension.ToLowerInvariant()
})
foreach($file in $vendored) {
  $violations.Add("Vendored binary dependency is not allowed in source: $($file.FullName)")
}

$packageManagers = @('vcpkg.json','conanfile.txt','conanfile.py','packages.config','package-lock.json','pnpm-lock.yaml','yarn.lock')
$packageManagerFiles = @(Get-ChildItem $SourceRoot -Recurse -File | Where-Object {
  $_.FullName -notmatch '[\\/](build|\.git)[\\/]' -and $_.Name -in $packageManagers
})

$commit = if($env:GITHUB_SHA){$env:GITHUB_SHA}else{try{(& git rev-parse HEAD 2>$null).Trim()}catch{"unknown"}}
$status = if($violations.Count -eq 0){"PASS"}else{"FAIL"}
$report = [ordered]@{
  schema=1
  generated_at_utc=[DateTime]::UtcNow.ToString('o')
  commit=$commit
  result=$status
  dependency_model="Windows SDK/system libraries plus in-repository ZeroSdk; no external package manager"
  linked_dependencies=@($linked | Sort-Object)
  allowed_system_libraries=$allowedSystemLibraries
  package_manager_manifests=@($packageManagerFiles | ForEach-Object {$_.FullName})
  vendored_binary_count=$vendored.Count
  violations=$violations.ToArray()
}
$jsonPath=Join-Path $ReportDir 'dependency-surface.json'
$mdPath=Join-Path $ReportDir 'dependency-surface.md'
$report|ConvertTo-Json -Depth 6|Set-Content $jsonPath -Encoding UTF8
$lines=New-Object System.Collections.Generic.List[string]
$lines.Add('# ZERO Dependency Surface Report');$lines.Add('');$lines.Add("- Result: **$status**");$lines.Add("- Commit: ``$commit``");$lines.Add("- Vendored binaries: $($vendored.Count)");$lines.Add("- External package-manager manifests: $($packageManagerFiles.Count)");$lines.Add('');$lines.Add('## Linked dependencies')
foreach($library in @($linked|Sort-Object)){$lines.Add("- $library")}
$lines.Add('');$lines.Add('## Violations')
if($violations.Count -eq 0){$lines.Add('None.')}else{foreach($v in $violations){$lines.Add("- $v")}}
$lines|Set-Content $mdPath -Encoding UTF8
Write-Host "ZERO dependency surface result: $status"
if($violations.Count -gt 0){exit 2}
exit 0
