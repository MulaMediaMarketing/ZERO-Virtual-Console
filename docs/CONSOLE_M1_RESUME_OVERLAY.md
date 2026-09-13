# ZERO Console Experience M1 — Resume + Overlay Slice

This slice advances ZERO from displaying Resume metadata to actually delivering Resume launch context into games through the ZERO SDK.

## Added

- explicit Play vs Resume choice on Game Detail
- Home defaults to Resume when a valid Resume point exists
- launch Resume context propagated Runtime V4 -> Runtime V3 -> ZERO SDK bootstrap
- payload-safe hexadecimal bootstrap encoding
- `Client::LaunchResume()` SDK access
- controller Menu-triggered ZERO system overlay
- overlay Continue / Achievements / Exit Game actions
- active session, playtime, and achievement counts in overlay
- recent games row based on persisted last-played timestamps
- aspect-correct center-cropped hero artwork
- validation game records received Resume launch payload

## Resume contract

Games continue to publish logical Resume state through `SetResumeActivity`. When the player chooses Resume, ZERO loads the stored activity and provides it to the new process during SDK initialization. The game is responsible for interpreting its own payload and restoring the correct logical state.

ZERO does not snapshot process RAM.

## Overlay boundary

The current overlay is implemented by temporarily raising the ZERO shell above the active game and notifying the game through the existing overlay-focus IPC contract. This is sufficient for Milestone 1 validation, but it is not yet a compositor-level or injected overlay suitable for every exclusive-fullscreen renderer.

## Remaining Milestone 1 work

- first-boot flow
- installer/uninstaller productization
- richer Library cards and icon/logo artwork
- controller focus polish and transition animation
- dedicated reference game acceptance pass
- overlay compatibility hardening for fullscreen presentation modes
