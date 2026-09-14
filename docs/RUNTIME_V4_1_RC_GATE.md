# ZERO Runtime V4.1 Release Candidate Gate

Runtime V4.1 is not considered accepted, production-ready, or RC-qualified from source compilation or CI alone.

## Automated M1 gate

Every PR and `main` build must:

1. configure and compile the Windows x64 Release build;
2. build the dedicated ZERO Reference Experience package;
3. run `tools/verify-m1-automated.ps1`;
4. produce both `m1-automated-acceptance.json` and `m1-automated-acceptance.md` evidence;
5. upload the shell, acceptance tools, reference package, installer/uninstaller, and acceptance evidence as one Windows artifact.

The automated runner executes the release-engineering gates for:

- Runtime IPC persistence ACK behavior;
- launch-time package integrity verification;
- real package-signature/trust verification;
- safe installed-package repair/re-import;
- controller-input state-machine behavior;
- shell/overlay motion and focus lifecycle;
- installer/update/uninstall preservation behavior;
- RC payload completeness;
- locked reference-package manifest and integrity contract.

The JSON report deliberately includes `rc_qualified=false`. A green automated report proves the tested software contracts passed in CI. It does not prove the supported real-machine hardware experience.

## Real Windows acceptance gate

Run on a clean Windows 11 x64 machine with a physical XInput-compatible controller connected.

1. Install ZERO with `install-zero.ps1`.
2. Launch ZERO and complete First Boot using the physical controller for navigation and confirmation.
3. Verify display and audio setup, then restart ZERO and confirm First Boot does not recur.
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
20. Run the installed `ZeroAcceptance.exe`.
21. Every interactive acceptance check must print PASS and the process must exit 0.
22. Run the M1 automated acceptance runner locally against the release build and archive its JSON/Markdown evidence with the hardware test record.
23. Uninstall ZERO normally and verify player saves/library/settings remain as documented.
24. Reinstall/upgrade ZERO and verify preserved data is still available.

## Required acceptance evidence

A qualification record must include:

- ZERO commit/build identifier;
- Windows 11 build and x64 architecture;
- physical controller make/model and successful XInput detection;
- generated M1 automated JSON + Markdown reports;
- installed `ZeroAcceptance.exe` output;
- package integrity verification result;
- launch/READY and Resume round-trip evidence;
- persisted achievement and canonical SQLite evidence;
- crash JSON and non-empty minidump evidence;
- restart-persistence result;
- uninstall/reinstall preservation result;
- tester PASS/FAIL decision for controller navigation, overlay behavior, and foreground recovery.

## Explicit non-claims

This gate does not certify:

- hostile-code sandboxing;
- Store DRM or online entitlement enforcement;
- anti-cheat;
- Store commerce;
- public cloud identity/social services;
- native capture recording/encoding;
- HDR or hardware certification;
- ARM64 or non-Windows platforms.

Publisher signature verification exists, but RC qualification does not imply a public publisher CA, production publisher portal, or Store signing service.

Only after the automated M1 gate and the full real-machine acceptance journey pass may Runtime V4.1 be called RC-qualified.
