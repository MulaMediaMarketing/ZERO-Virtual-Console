param([string]$SourceRoot = ".")
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$violations = New-Object System.Collections.Generic.List[string]
function Need([string]$path,[string]$pattern,[string]$message) {
  $text = Get-Content (Join-Path $SourceRoot $path) -Raw
  if ($text -notmatch $pattern) { $violations.Add("$path :: $message") }
}
function Forbid([string]$path,[string]$pattern,[string]$message) {
  $text = Get-Content (Join-Path $SourceRoot $path) -Raw
  if ($text -match $pattern) { $violations.Add("$path :: $message") }
}

Need "src/AppPages.cpp" 'sidebarWidth\s*=\s*230\.0f' "locked left sidebar missing"
Need "src/AppPages.cpp" '0x060A11' "dark cinematic background missing"
Need "src/AppPages.cpp" 'ZERO PLAYER SERVICE' "service-state presentation missing"
Need "src/AppPages.cpp" 'NOTIFICATIONS' "top system bar notifications affordance missing"
Need "src/AppPages.cpp" 'DOWNLOADS' "top system bar download affordance missing"
Need "src/AppPages.cpp" 'LOCAL ZERO ID|ZERO ID ONLINE' "account/system identity state missing"
Need "src/AppUx.cpp" '0x20CFFF' "cyan keyboard/controller focus ring missing"
Forbid "src/AppPages.cpp" '0xFBFAF7' "legacy light shell background is forbidden"
Forbid "src/AppPages.cpp" 'const\s+float\s+navStart' "legacy top-navigation renderer is forbidden"
Forbid "src/AppSettings.cpp" 'Runtime V4\.1' "stale pre-cutover runtime identity is forbidden"

# Bootstrap is composition-only. UI/window behavior belongs to App/Shell modules.
Need "src/main.cpp" 'ApplicationServices\s+services\s*\(' "bootstrap must create the production composition root"
Need "src/main.cpp" 'App\s+app\s*\(instance,\s*services\)' "bootstrap must inject services into App"
Forbid "src/main.cpp" 'CreateWindowExW|TrackPopupMenu|drawSidebar|ZeroProductionHeaderOverlay' "main.cpp must remain bootstrap-only"

# The native production window remains recoverable and owns standard desktop behavior.
Need "src/App.cpp" 'WS_OVERLAPPEDWINDOW\s*\|\s*WS_CLIPCHILDREN\s*\|\s*WS_CLIPSIBLINGS' "App must create standard Windows chrome natively"
Need "src/App.cpp" 'ShowWindow\(hwnd_,\s*SW_MAXIMIZE\)' "initial presentation must maximize a standard window"
Need "src/App.cpp" 'WM_GETMINMAXINFO' "native App window must own minimum-size enforcement"
Need "src/App.cpp" 'ptMinTrackSize\.x\s*=\s*1280' "native App minimum width must remain 1280 pixels"
Need "src/App.cpp" 'ptMinTrackSize\.y\s*=\s*720' "native App minimum height must remain 720 pixels"
Need "src/App.cpp" 'ShowWindow\(hwnd_,\s*IsZoomed\(hwnd_\)\s*\?\s*SW_RESTORE\s*:\s*SW_MAXIMIZE\)' "F11 must toggle maximize/restore while preserving standard chrome"
Forbid "src/App.cpp" 'CreateWindowExW[\s\S]{0,240}WS_POPUP' "App must not create the production shell as a popup window"
Forbid "src/AppUx.cpp" 'EnterBorderlessFullscreen\(\)' "foreground restoration must preserve the user window state instead of forcing fullscreen"
Need "src/AppUx.cpp" 'IsIconic\(hwnd_\)' "foreground restoration must recover a minimized window safely"
Need "src/App.cpp" 'x\s*>=\s*8\s*&&\s*x\s*<=\s*214' "native sidebar mouse hitbox must include the complete icon lane"

Need "include/PlaceholderContentProvider.h" 'covers/echoes-of-tomorrow\.svg' "preview provider asset extension must match committed artwork"
Need "include/PlaceholderContentProvider.h" 'hero-echoes-of-tomorrow\.svg' "preview hero extension must match committed artwork"
Need ".github/workflows/windows-build.yml" 'build/Release/assets/placeholders/\*\*' "Windows artifact must package removable preview fixtures beside the executable"

foreach($name in @('Discover','CloudPlay','Downloads','Profile','Devices','Wishlist','Checkout','Notifications')) {
  Need "src/v5/ProductionShellIntegration.cpp" "ShellPage::$name, true" "$name is not reachable in production shell"
}

Need "src/AppPages.cpp" 'Page::Discover\) DrawDiscover' "Discover must render the local-first production experience"
Need "src/AppPages.cpp" 'Page::Downloads\) DrawDownloads' "Downloads must render the local-first production experience"
Need "src/AppPages.cpp" 'Page::Profile\) DrawProfile' "Profile must render the local-first production experience"
Need "src/AppPages.cpp" 'Page::Devices\) DrawDevices' "Devices must render the local-first production experience"
Need "src/AppPages.cpp" 'Page::Notifications\) DrawNotifications' "Notifications must render the local-first production experience"
Need "src/AppNextLevelPages.cpp" 'void App::DrawProfile' "Profile renderer implementation missing"
Need "src/AppNextLevelPages.cpp" 'void App::DrawDownloads' "Downloads renderer implementation missing"
Need "src/AppNextLevelPages.cpp" 'downloads_\.Queue\(\)' "Downloads must project the live V5 download queue"
Need "include/App.h" 'v5::DownloadAuthority&\s+downloads_' "App must reference the injected V5 download authority"
Need "src/AppNextLevelPages.cpp" 'void App::DrawDevices' "Devices renderer implementation missing"
Need "src/AppNextLevelPages.cpp" 'void App::DrawNotifications' "Notifications renderer implementation missing"
Need "src/AppNextLevelPages.cpp" 'void App::DrawDiscover' "Discover renderer implementation missing"
Need "src/AppAchievements.cpp" 'YOUR VERIFIED ACHIEVEMENT HISTORY' "top-level Achievements dashboard missing"
Need "src/AppAchievements.cpp" 'AchievementDefinitions' "Achievements must consume authoritative definitions when available"
Need "src/AppAchievements.cpp" 'Secret achievement' "secret-achievement presentation missing"
Need "src/AppAchievements.cpp" 'ZERO SCORE' "authoritative ZERO Score presentation missing"
Need "CMakeLists.txt" 'src/AppNextLevelPages\.cpp' "next-level page renderer must be linked into ZERO Player"

Need "include/Settings.h" 'shareActivity\{false\}' "activity privacy must default private"
Need "include/Settings.h" 'shareAchievements\{false\}' "achievement privacy must default private"
Need "include/Settings.h" 'sharePlaytime\{false\}' "playtime privacy must default private"
Need "src/Settings.cpp" 'share_activity' "activity privacy persistence missing"
Need "src/Settings.cpp" 'share_achievements' "achievement privacy persistence missing"
Need "src/Settings.cpp" 'share_playtime' "playtime privacy persistence missing"
Need "src/AppSettings.cpp" 'SettingsExperienceRow::Privacy' "privacy controls missing from live Settings"
Need "src/AppSettings.cpp" 'ZERO Core V5' "live About state must identify the V5 platform"

Need "src/Input.cpp" 'XInputGetState' "controller input path missing"
Need "src/Input.cpp" 'VK_UP' "keyboard directional input path missing"
Need "src/App.cpp" 'WM_LBUTTONDOWN' "mouse sidebar navigation path missing"
Need "src/App.cpp" 'productionShell_\.MoveTopLevel' "controller/keyboard top-level navigation must route through ShellKernel"
Need "src/App.cpp" 'productionShell_\.Back' "Back must route through ShellKernel"
Need "src/App.cpp" 'GET_X_LPARAM' "mouse pointer x coordinate routing missing"
Need "src/App.cpp" 'GET_Y_LPARAM' "mouse pointer y coordinate routing missing"

if($violations.Count -gt 0) {
  Write-Host "ZERO Player locked UX gate: FAIL"
  foreach($item in $violations){ Write-Host "UX VIOLATION: $item" }
  exit 2
}
Write-Host "ZERO Player locked UX gate: PASS"
exit 0
