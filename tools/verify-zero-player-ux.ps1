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

foreach($name in @('Discover','CloudPlay','Downloads','Profile','Devices','Wishlist','Checkout','Notifications')) {
  Need "src/v5/ProductionShellIntegration.cpp" "ShellPage::$name, true" "$name is not reachable in production shell"
}

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
