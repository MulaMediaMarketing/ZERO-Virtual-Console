# ZERO Console Experience Milestone 1 Acceptance Runbook

This runbook defines the release gate for ZERO Console Experience Milestone 1. A green compiler build is necessary but not sufficient. Milestone 1 is accepted only when this complete journey passes on a real Windows 11 x64 machine using the packaged CI artifact.

## Release standard

Do not label Milestone 1 production-ready, AAA, accepted, or RC-qualified unless every required check below passes without manual file repair, hidden state edits, or bypasses.

## Test package

Use the generated `ReferencePackage` from the Windows CI artifact. It contains:

- `ZeroReferenceGame.exe`
- `zero.manifest.json`
- production reference hero artwork
- icon artwork
- logo artwork

Package ID: `zero.system.reference`

## Clean-machine journey

1. Start from a user account with no `%LOCALAPPDATA%\ZERO` state.
2. Install ZERO using the packaged installer.
3. Launch ZERO.
4. Complete the full native First Boot flow with a controller.
5. Exit ZERO and launch it again.
6. Confirm First Boot does not appear a second time.
7. Import the generated `ReferencePackage` through ZERO.
8. Confirm the package appears in Library with artwork.
9. Launch the package.
10. Confirm the SDK READY handshake completes.
11. Close the reference game normally.
12. Confirm Home/Game Detail exposes the saved Resume activity.
13. Choose Resume.
14. The reference game must validate the returned activity ID and payload and write `resume_roundtrip.ok` under its managed save root.
15. Open the ZERO overlay while the game is active and verify Continue and Exit Game behavior.
16. Exit cleanly back to ZERO.
17. Create `%LOCALAPPDATA%\ZERO\Saves\zero.system.reference\force_crash.next`.
18. Launch the reference package once more.
19. It must intentionally exit with code `73` after SDK READY.
20. ZERO must remain alive and return to its shell.
21. A crash diagnostic must exist under `%LOCALAPPDATA%\ZERO\CrashReports\zero.system.reference\`.
22. Run `ZeroAcceptance.exe`.
23. Every check must report `PASS` and the process must return exit code `0`.
24. Uninstall ZERO with the normal uninstall path.
25. Confirm `%LOCALAPPDATA%\ZERO\Saves` remains intact.

## Acceptance checks enforced by ZeroAcceptance.exe

- First Boot completion and controller/display/audio confirmation
- reference package installed through the managed Library
- reference game launch marker
- SDK READY marker
- Runtime session persistence
- Resume registration
- achievement persistence
- persisted Resume metadata contents
- Resume payload round-trip back into the game
- intentional crash trigger marker
- package-specific crash diagnostics

Directory existence alone is not considered sufficient evidence.

## Failure policy

Any failed step blocks RC1. Fix the defect, rebuild the artifact, restart the clean-machine journey, and re-run the complete acceptance suite. Do not waive failures for UI, controller behavior, persistence, crash recovery, install/uninstall, Resume, or SDK communication.

## RC1 criteria

Milestone 1 RC1 may be cut only after:

- PR CI is green
- `main` CI is green
- packaged artifact contains shell, acceptance executable, installer scripts, and ReferencePackage
- the full real-machine run above passes
- `ZeroAcceptance.exe` returns `PASS`
- uninstall preserves player data by default

After RC1 acceptance, development proceeds to Runtime V4.1 hardening rather than adding broad new platform features.
