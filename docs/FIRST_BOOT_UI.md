# ZERO Native First Boot — Milestone 1

This slice closes the visual First Boot gap in Console Experience Milestone 1.

## Startup behavior

`wWinMain` creates `FirstBootService` against `%LOCALAPPDATA%\ZERO`.

If First Boot is incomplete, ZERO launches the native fullscreen `FirstBootWizard` before the main console shell.

After the wizard persists a completed state, subsequent boots skip setup and enter the normal ZERO Home experience.

## Controller-first setup flow

1. Welcome
2. Local ZERO profile confirmation
3. Controller confirmation
4. Display confirmation
5. Starting system-volume selection
6. Ready / enter Home

Xbox-compatible controller input is supported through XInput. Keyboard input remains available as the development/accessibility fallback.

## Persistence

The wizard writes through `FirstBootService`; UI code does not own the persistence format.

The state records:
- setup completed
- profile name
- controller confirmed
- display confirmed
- audio confirmed

## Milestone acceptance boundary

This closes the source-level First Boot UI gap. It does **not** by itself prove the full Milestone 1 hardware journey.

Final acceptance still requires executing the packaged Windows build on a real Windows machine and verifying:

- clean install
- fullscreen First Boot
- controller navigation
- game import
- artwork / Library presentation
- launch + READY handshake
- achievement unlock
- Resume creation
- overlay interaction
- exit back to ZERO
- Resume payload delivery on relaunch
- intentional game crash while ZERO survives
- crash diagnostics creation
- `ZeroAcceptance.exe` result
- uninstall while player data is preserved

ZERO should only be called Milestone-1 accepted after that full journey passes.
