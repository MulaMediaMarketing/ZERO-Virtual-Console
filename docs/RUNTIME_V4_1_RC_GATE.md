# ZERO Runtime V4.1 Release Candidate Gate

Runtime V4.1 is not considered accepted, production-ready, or RC-qualified from source compilation or CI alone.

## Automated M1 gate

Every PR and `main` build must:

1. configure and compile the Windows x64 Release build;
2. build the dedicated ZERO Reference Experience package;
3. run `tools/verify-m1-automated.ps1`;
4. produce both `m1-automated-acceptance.json` and `m1-automated-acceptance.md` evidence;
5. run `tools/run-rc-qualification.ps1 -NonInteractiveValidation` and prove CI remains `PENDING`, never qualified;
6. upload the shell, acceptance tools, reference package, installer/uninstaller, qualification probe, and acceptance evidence as one Windows artifact.

The automated runner covers Runtime IPC persistence, package integrity, publisher trust verification, safe repair/re-import, controller input logic, shell/overlay lifecycle, installer/update/uninstall behavior, RC payload completeness, and the locked reference-package contract.

The automated report deliberately includes `rc_qualified=false`. A green CI report proves tested software contracts passed. It does not prove the supported real-machine experience.

## Qualification tooling

PR #43 freezes the RC evidence path around two tools:

- `ZeroQualificationProbe.exe` records only non-sensitive qualification facts: Windows build, native architecture, and XInput slot detection. It does not record the Windows machine name or user identity.
- `tools/run-rc-qualification.ps1` consumes the automated M1 report plus explicit hardware/manual results and emits `rc-qualification.json` and `rc-qualification.md`.

CI invokes the qualification runner only with `-NonInteractiveValidation`. In that mode it must output `qualification_result=PENDING` and `rc_qualified=false`. CI is structurally prevented from qualifying the release candidate.

## Real Windows acceptance gate

Run on a clean Windows 11 x64 machine with a physical XInput-compatible controller connected.

1. Install ZERO with `install-zero.ps1`.
2. Launch ZERO and complete First Boot using the physical controller for navigation and confirmation.
3. Verify display and audio setup, restart ZERO, and confirm First Boot does not recur.
4. Traverse Home, Library, Game Detail, Settings, Achievements, Captures, overlay open/close, and Back/Home navigation entirely with the controller.
5. Disconnect and reconnect the controller during shell navigation and verify focus remains recoverable without phantom A/B/Menu actions.
6. Import the generated `ReferencePackage` through ZERO.
7. Confirm the installed package is finalized with `zero.integrity.sha256`.
8. Launch the reference game and confirm SDK READY.
9. Open and close the ZERO overlay over the running game and verify shell/game focus restoration.
10. Exit normally and confirm the shell returns to borderless fullscreen and remains responsive.
11. Confirm Resume is offered.
12. Launch through Resume and verify the reference round-trip marker is written.
13. Confirm the reference achievement persists.
14. Confirm `%LOCALAPPDATA%\ZERO\Data\zero.db` contains canonical game, session, game_stats, Resume, and achievement rows for `zero.system.reference`.
15. Restart Windows and verify the installed library, settings, Resume data, achievements, and save state persist.
16. Create `%LOCALAPPDATA%\ZERO\Saves\zero.system.reference\force_crash.next`.
17. Launch the reference game and allow its deliberate unhandled exception to occur.
18. Verify ZERO remains alive and restores the shell foreground/fullscreen experience.
19. Verify a non-empty `.dmp` and package-specific crash JSON exist under `%LOCALAPPDATA%\ZERO\CrashReports\zero.system.reference`.
20. Run the installed `ZeroAcceptance.exe` and require every check to print PASS with process exit code 0.
21. Run the M1 automated acceptance runner locally against the exact release build.
22. Uninstall ZERO normally and verify player saves/library/settings remain as documented.
23. Reinstall/upgrade ZERO and verify preserved data is still available.
24. Run the RC qualification command below using the real controller make/model and only the switches whose journeys actually passed.

Example only after every listed result has been physically observed:

```powershell
./tools/run-rc-qualification.ps1 `
  -BuildRoot ./build `
  -EvidenceDir ./build/QualificationEvidence `
  -ControllerModel "<actual controller make/model>" `
  -FirstBootPassed `
  -ControllerTraversalPassed `
  -OverlayFocusPassed `
  -ForegroundRecoveryPassed `
  -RestartPersistencePassed `
  -UninstallReinstallPassed `
  -CrashContainmentPassed `
  -ResumeRoundTripPassed `
  -CanonicalStatePassed
```

The command exits 0 and writes `rc_qualified=true` only when all automated, OS/architecture, physical-controller, controller-identification, and manual qualification requirements are satisfied. Missing evidence returns a non-zero exit and a `PENDING` report with explicit blockers.

## Required acceptance evidence

A qualification record must include:

- exact ZERO commit/build identifier and version `0.1.0-rc1`;
- Windows 11 build and x64 architecture;
- physical controller make/model and successful XInput detection;
- generated M1 automated JSON + Markdown reports;
- generated RC qualification JSON + Markdown reports;
- installed `ZeroAcceptance.exe` output;
- package integrity verification result;
- launch/READY and Resume round-trip evidence;
- persisted achievement and canonical SQLite evidence;
- crash JSON and non-empty minidump evidence;
- restart-persistence result;
- uninstall/reinstall preservation result;
- tester PASS/FAIL decision for controller navigation, overlay behavior, and foreground recovery.

## Release freeze rule

After the exact candidate build receives a complete `rc_qualified=true` qualification record, no source, build-script, installer, manifest, runtime, shell, or SDK change may be made to that candidate without invalidating the record and rerunning qualification against the new commit.

Do not rewrite, hand-edit, or promote a `PENDING` qualification report to PASS. Qualification comes from rerunning the tooling against the exact candidate build after the real-machine journey succeeds.

## Explicit non-claims

This gate does not certify hostile-code sandboxing, Store DRM or online entitlements, anti-cheat, Store commerce, public cloud identity/social services, native capture recording/encoding, HDR/hardware certification, ARM64, or non-Windows platforms.

Publisher signature verification exists, but RC qualification does not imply a public publisher CA, production publisher portal, or Store signing service.

Only after the automated M1 gate and the full real-machine acceptance journey pass for the exact candidate commit may Runtime V4.1 be called RC-qualified.
