param(
  [string]$BuildRoot = "./build",
  [string]$OutputDir = "./build/ReleaseBundle"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
function Require-File([string]$Path) { if(-not(Test-Path $Path -PathType Leaf)){throw "Missing release file: $Path"};if((Get-Item $Path).Length -le 0){throw "Empty release file: $Path"} }

$files=@(
 "Release/ZeroVirtualConsole.exe","Release/ZeroTestGame.exe","Release/ZeroReferenceGame.exe","Release/ZeroAcceptance.exe",
 "Release/ZeroIpcPersistenceAcceptance.exe","Release/ZeroPackageIntegrityAcceptance.exe","Release/ZeroPackageTrustAcceptance.exe","Release/ZeroPackageRepairAcceptance.exe",
 "Release/ZeroStrictJsonAcceptance.exe","Release/ZeroPlatformDatabaseAcceptance.exe","Release/ZeroFaultInjectionAcceptance.exe","Release/ZeroProductionBenchmarkAcceptance.exe",
 "Release/ZeroInputAcceptance.exe","Release/ZeroShellUxAcceptance.exe","Release/ZeroProductionUxAcceptance.exe","Release/ZeroFriendsExperienceAcceptance.exe",
 "Release/ZeroCapturesExperienceAcceptance.exe","Release/ZeroCoreShellExperienceAcceptance.exe","Release/ZeroStoreSettingsFirstBootAcceptance.exe","Release/ZeroFirstBootPersistenceAcceptance.exe",
 "Release/ZeroQualificationProbe.exe","ReferencePackage/ZeroReferenceGame.exe","ReferencePackage/zero.manifest.json","ReferencePackage/zero.integrity.sha256",
 "DependencyReports/dependency-surface.json","DependencyReports/dependency-surface.md","ArchitectureReports/production-architecture.json","ArchitectureReports/production-architecture.md",
 "AcceptanceReports/m1-automated-acceptance.json","AcceptanceReports/m1-automated-acceptance.md","QualificationEvidence/rc-qualification.json","QualificationEvidence/rc-qualification.md",
 "SBOM/zero-virtual-console.spdx.json"
)
if(Test-Path $OutputDir){Remove-Item -Recurse -Force $OutputDir};New-Item -ItemType Directory -Force -Path $OutputDir|Out-Null
$manifestEntries=New-Object System.Collections.Generic.List[object]
foreach($relative in $files){
 $source=Join-Path $BuildRoot $relative;Require-File $source;$destination=Join-Path $OutputDir $relative
 New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination)|Out-Null;Copy-Item -Force $source $destination
 $manifestEntries.Add([pscustomobject]@{path=$relative.Replace('\\','/');sha256=(Get-FileHash -Algorithm SHA256 $destination).Hash.ToLowerInvariant();bytes=(Get-Item $destination).Length})
}
$dependency=Get-Content (Join-Path $BuildRoot "DependencyReports/dependency-surface.json") -Raw|ConvertFrom-Json
$architecture=Get-Content (Join-Path $BuildRoot "ArchitectureReports/production-architecture.json") -Raw|ConvertFrom-Json
$acceptance=Get-Content (Join-Path $BuildRoot "AcceptanceReports/m1-automated-acceptance.json") -Raw|ConvertFrom-Json
$qualification=Get-Content (Join-Path $BuildRoot "QualificationEvidence/rc-qualification.json") -Raw|ConvertFrom-Json
if($dependency.result -ne "PASS"){throw "Dependency surface gate did not pass."};if($architecture.result -ne "PASS"){throw "Production architecture gate did not pass."};if($acceptance.automated_result -ne "PASS"){throw "Automated M1 acceptance did not pass."};if($qualification.rc_qualified -eq $true){throw "CI/release packaging must not promote itself to physically qualified."}
$commit=if($env:GITHUB_SHA){$env:GITHUB_SHA}else{try{(& git rev-parse HEAD 2>$null).Trim()}catch{"unknown"}}
$manifest=[ordered]@{schema=6;product="ZERO Virtual Console";milestone="M1 / Runtime V4.1 / Production Architecture";release_class="release-candidate";commit=$commit;generated_at_utc=[DateTime]::UtcNow.ToString("o");dependency_result=$dependency.result;dependency_model=$dependency.dependency_model;architecture_result=$architecture.result;automated_acceptance_result=$acceptance.automated_result;architecture_metrics=$architecture.source_metrics;acceptance_contract_metrics=$architecture.acceptance_metrics;acceptance_performance_metrics=$acceptance.performance_metrics;shell_metric=$architecture.shell_metric;sbom="SBOM/zero-virtual-console.spdx.json";rc_qualified=$false;rc_qualification_note="Bundle is release-candidate material only until physical Windows 11 x64 and controller qualification passes for this exact commit.";files=$manifestEntries.ToArray()}
$manifestPath=Join-Path $OutputDir "release-manifest.json";$manifest|ConvertTo-Json -Depth 12|Set-Content -Encoding UTF8 -Path $manifestPath
$zipPath=Join-Path (Split-Path -Parent $OutputDir) "ZERO-Virtual-Console-Windows-x64-RC.zip";if(Test-Path $zipPath){Remove-Item -Force $zipPath};Compress-Archive -Path (Join-Path $OutputDir '*') -DestinationPath $zipPath -CompressionLevel Optimal;Require-File $zipPath
Write-Host "ZERO release bundle: $OutputDir";Write-Host "ZERO release manifest: $manifestPath";Write-Host "ZERO RC archive: $zipPath";Write-Host "ZERO dependency surface: $($dependency.result)";Write-Host "ZERO production architecture: $($architecture.result)";Write-Host "ZERO automated acceptance: $($acceptance.automated_result)";Write-Host "ZERO acceptance runtime: $($acceptance.performance_metrics.total_acceptance_duration_ms) ms";Write-Host "ZERO RC qualification: PENDING PHYSICAL ACCEPTANCE"
