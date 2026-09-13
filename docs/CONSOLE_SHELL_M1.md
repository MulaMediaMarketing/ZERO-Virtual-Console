# ZERO Console Experience Milestone 1 — Native Shell Slice

This slice upgrades the ZERO shell from a desktop-window MVP toward a controller-first console surface.

## Added

- Borderless fullscreen boot on the active monitor.
- Native Import page in the top navigation.
- Windows folder picker for selecting a ZERO-compatible game folder.
- Import flow routed through `GameImportService` so packages are validated and staged before entering the Library.
- WIC-based hero artwork decoding and Direct2D rendering.
- UTF-8 title rendering in the shell.
- Resume awareness on Home and Game Detail pages using existing `ResumeStore` metadata.
- Platform playtime and launch count surfaced on Game Detail.
- Improved crash status messaging consistent with Runtime V4 diagnostics.

## Controller journey

`Home -> Library / Import -> Select game -> Game Detail -> Play or Resume`

Import can be reached from the top navigation with the existing left/right/select controller abstraction. The Windows folder picker is still OS-native and will receive additional controller UX hardening later.

## Important boundary

The shell can now identify and present an available Resume activity, but the stored resume payload is not yet injected back into the game launch. Resume launch-context delivery remains a separate runtime/SDK task and must not be considered complete until the game receives and acknowledges that payload.

## Next shell work

1. Resume launch context delivery through Runtime/SDK.
2. Aspect-correct artwork crop instead of simple rectangle scaling.
3. Game logo/icon rendering and recent-played rows.
4. Native ZERO overlay UI.
5. First-boot setup and installer productization.
6. Controller-first replacement or wrapper for the OS folder picker.
