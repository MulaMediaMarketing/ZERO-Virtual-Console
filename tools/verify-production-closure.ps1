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

# Composition root and App ownership.
RequirePresent "include/ApplicationServices.h" 'class\s+ApplicationServices\s+final' "production composition root is missing"
RequirePresent "src/main.cpp" 'ApplicationServices\s+services\s*\(' "bootstrap must construct the production composition root"
RequirePresent "src/main.cpp" 'App\s+app\s*\(instance,\s*services\)' "bootstrap must inject ApplicationServices into App"
RequirePresent "include/App.h" 'App\(HINSTANCE\s+instance,\s*ApplicationServices&\s+services\)' "App must receive production services through dependency injection"
RequirePresent "include/App.h" 'ApplicationServices&\s+services_' "App must retain the injected composition root"
RequireAbsent "include/App.h" '\bLocalIdentityProvider\s+identity_' "App may not construct/own the concrete local identity provider"
RequireAbsent "include/App.h" '\bDisconnectedFriendsProvider\s+friends_' "App may not construct/own the concrete friends transport"
RequireAbsent "include/App.h" '\bDisconnectedStoreProvider\s+store_' "App may not construct/own the concrete Store transport"
RequireAbsent "include/App.h" '(?m)^\s*GameRegistry\s+registry_;' "App may not own the concrete game registry"
RequireAbsent "include/App.h" '(?m)^\s*v5::ImportCoordinator\s+importer_;' "App may not own the concrete import coordinator"
RequireAbsent "include/App.h" '(?m)^\s*v5::DownloadAuthority\s+downloads_;' "App may not own the concrete download authority"
RequireAbsent "include/App.h" '(?m)^\s*ProductionRuntime\s+runtime_;' "App may not own the concrete runtime"
RequireAbsent "include/App.h" '(?m)^\s*SettingsStore\s+settingsStore_;' "App may not own the concrete settings store"
RequirePresent "include/App.h" 'v5::ImportCoordinator&\s+importer_' "App import projection must reference the composition root authority"
RequirePresent "include/App.h" 'v5::DownloadAuthority&\s+downloads_' "App download projection must reference the composition root authority"
RequirePresent "include/App.h" 'ProductionRuntime&\s+runtime_' "App runtime projection must reference the composition root authority"
RequirePresent "include/App.h" 'v5::ProductionShellIntegration&\s+productionShell_' "ShellKernel integration must remain authoritative"
RequirePresent "include/ApplicationServices.h" 'v5::ImportCoordinator\s+importer_' "composition root must own the V5 import authority"
RequirePresent "include/ApplicationServices.h" 'v5::DownloadAuthority\s+downloads_' "composition root must own the V5 download authority"
RequirePresent "include/ApplicationServices.h" 'ProductionRuntime\s+runtime_' "composition root must own the production runtime"
RequirePresent "include/ApplicationServices.h" 'v5::ProductionShellIntegration\s+shell_' "composition root must own the production shell integration"
RequirePresent "include/application/ServiceStateCoordinator.h" 'void\s+RefreshForPage\s*\(ProductionUxPage\s+page\)' "ServiceStateCoordinator must own per-page refresh policy"
RequireAbsent "include/shell/ShellModule.h" 'RefreshForPage|RefreshAll' "ShellModule must remain navigation-only and may not own service refresh policy"
RequireAbsent "src/shell/ShellModule.cpp" 'registry_\.Refresh|captures_\.Refresh|friends_\.Refresh|store_\.Refresh' "ShellModule may not directly refresh service providers"
RequirePresent "src/App.cpp" 'productionShell_\.Navigate\(next,\s*error\)' "App navigation must commit through ProductionShellIntegration"
RequirePresent "src/App.cpp" 'ActivePage\(\)' "App navigation must verify the committed authoritative page"

# Legacy ownership must remain cut over.
RequireAbsent "include/App.h" '#include\s+"ResumeStore\.h"' "App may not include legacy ResumeStore"
RequireAbsent "include/App.h" '#include\s+"GameImportService\.h"' "App may not include GameImportService directly"
RequireAbsent "include/App.h" '\bResumeStore\s+[A-Za-z_][A-Za-z0-9_]*\s*[;{]' "App may not own a ResumeStore"
RequireAbsent "include/App.h" '\bGameImportService\s+[A-Za-z_][A-Za-z0-9_]*\s*[;{]' "App may not own GameImportService"
RequirePresent "include/App.h" 'AuthoritativeResumeProjection\s+resumeStore_;' "App resume reads must project through ProductionRuntime"
RequireAbsent "include/App.h" '(?m)^\s*Page\s+page_\s*\{' "Raw App page authority is forbidden"
RequireAbsent "include/App.h" '(?m)^\s*size_t\s+navIndex_\s*\{' "Raw App nav authority is forbidden"
RequirePresent "cmake/ZeroV5.cmake" 'src/v5/ImportCoordinator\.cpp' "V5 ImportCoordinator must be in production target"

RequireAbsent "include/v5/ProductionRuntime.h" '#include\s+"ResumeStore\.h"' "ProductionRuntime may not include legacy ResumeStore"
RequireAbsent "include/v5/ProductionRuntime.h" '#include\s+"PlatformStateStore\.h"' "ProductionRuntime may not include legacy PlatformStateStore"
RequireAbsent "include/v5/ProductionRuntime.h" '#include\s+"AchievementStore\.h"' "ProductionRuntime may not include legacy AchievementStore directly"
RequirePresent "include/v5/ProductionRuntime.h" 'LocalResumeRepository\s+resumes_' "Runtime Resume persistence must use V5 repository"
RequirePresent "include/v5/ProductionRuntime.h" 'LocalAchievementRepository\s+achievements_' "Runtime achievement persistence must use V5 repository"
RequirePresent "include/v5/ProductionRuntime.h" 'LocalSessionRepository\s+sessions_' "Runtime session persistence must use V5 repository"
RequireAbsent "src/v5/ProductionRuntime.cpp" 'resumeStore_\.' "ProductionRuntime may not use legacy ResumeStore"
RequireAbsent "src/v5/ProductionRuntime.cpp" 'stateStore_\.' "ProductionRuntime may not mirror/fall back to legacy platform state"
RequireAbsent "src/v5/ProductionRuntime.cpp" 'achievements_\.Unlock\(' "ProductionRuntime achievements must use repository Save contract"
RequirePresent "src/v5/ProductionRuntime.cpp" 'resumes_\.Save\(' "Runtime Resume writes must use V5 repository"
RequirePresent "src/v5/ProductionRuntime.cpp" 'sessions_\.Record\(' "Runtime sessions must use V5 repository"
RequirePresent "src/v5/ProductionRuntime.cpp" 'achievements_\.Save\(' "Runtime achievements must use repository Save contract"

# Locked product shell still applies during the structural refactor.
RequirePresent "src/AppPages.cpp" 'constexpr\s+float\s+sidebarWidth\s*=\s*230\.0f' "Locked UI requires permanent left sidebar"
RequirePresent "src/AppPages.cpp" 'target_->Clear\(D2D1::ColorF\(0x060A11\)\)' "Locked UI requires dark cinematic base"
RequirePresent "src/AppPages.cpp" '0x20CFFF|0x087DFF' "Locked UI requires blue/cyan focus/accent treatment"
foreach($page in @('Discover','CloudPlay','Downloads','Profile','Devices','Wishlist','Checkout','Notifications')) {
  RequirePresent "src/v5/ProductionShellIntegration.cpp" "ShellPage::$page, true" "$page must be reachable through production shell"
}
RequireAbsent "src/v5/ProductionShellIntegration.cpp" 'not integrated into the production renderer yet' "No production destination may remain renderer-unintegrated"

if ($violations.Count -gt 0) {
  Write-Host "ZERO production closure gate: FAIL"
  foreach ($item in $violations) { Write-Host "PRODUCTION CLOSURE VIOLATION: $item" }
  exit 2
}
Write-Host "ZERO production closure gate: PASS"
exit 0
