# ZERO Console Experience Milestone 1 — Finalization

This slice prepares ZERO for a repeatable local install and end-to-end acceptance pass.

## Added

- `FirstBootService` with persistent first-boot state for profile, controller, display, and audio confirmation.
- `ZeroAcceptance.exe`, an executable Milestone 1 verification utility.
- install/uninstall PowerShell scripts.
- uninstall preserves `%LOCALAPPDATA%\ZERO` player data by default.
- optional explicit player-data removal during uninstall.
- Windows CI publishes the shell, validation game, acceptance utility, and installer scripts together.

## First-boot contract

First boot is represented by `%LOCALAPPDATA%\ZERO\first_boot.ini` and is incomplete until all required confirmations are recorded:

- profile
- controller
- display
- audio

`FirstBootService` is intentionally separate from the renderer so the onboarding UX can change without changing persistence semantics.

## Acceptance utility

Run `ZeroAcceptance.exe` after the manual acceptance journey. It currently verifies that the expected durable platform artifacts exist:

- first-boot state
- Library root
- managed Saves root
- Resume metadata root
- Runtime session records
- CrashReports root

The utility exits `0` only when all checks pass and exits `2` when the milestone remains incomplete.

## Manual Milestone 1 journey

1. Install ZERO.
2. Complete first boot.
3. Navigate ZERO with a controller.
4. Import a compatible game.
5. Confirm artwork and metadata in Library/Game Detail.
6. Launch the game.
7. Confirm authenticated SDK and READY.
8. Unlock an achievement.
9. Set a logical Resume activity.
10. Open the ZERO overlay.
11. Exit back to ZERO.
12. Resume the game and confirm the game receives the Resume context.
13. Intentionally crash the reference game and confirm ZERO remains alive.
14. Run `ZeroAcceptance.exe`.
15. Uninstall ZERO without `-RemovePlayerData` and confirm Saves/Library/Resume data remain.

## Remaining before calling Milestone 1 fully accepted

The branch compiles the persistence and acceptance services, but the visual first-boot wizard still needs to be wired into the native shell. Milestone 1 should not be called complete until that UI path and the full journey above are executed on a real Windows machine.
