# ZERO Runtime V4.1 Release Candidate Gate

Runtime V4.1 is not considered accepted, production-ready, or RC-qualified from source compilation alone.

## Automated CI gate

Every PR and `main` build must:

1. configure and compile the Windows x64 Release build;
2. build the dedicated ZERO Reference Experience package;
3. pass `tools/verify-rc-build.ps1`;
4. upload a complete artifact containing the shell, acceptance executable, reference package, installer, and uninstaller.

## Real Windows acceptance gate

Run on a clean Windows 11 x64 machine with a controller connected.

1. Install ZERO with `install-zero.ps1`.
2. Launch ZERO and complete First Boot using controller navigation.
3. Restart ZERO and verify First Boot does not recur.
4. Import the generated `ReferencePackage` through ZERO.
5. Confirm the package is finalized with `zero.integrity.sha256`.
6. Launch the reference game and confirm SDK READY.
7. Exit normally and confirm the shell remains alive.
8. Confirm Resume is offered.
9. Launch through Resume and verify the round-trip marker is written.
10. Confirm the reference achievement persists.
11. Confirm `%LOCALAPPDATA%\ZERO\Data\zero.db` contains canonical game, session, game_stats, Resume, and achievement rows for `zero.system.reference`.
12. Create `%LOCALAPPDATA%\ZERO\Saves\zero.system.reference\force_crash.next`.
13. Launch the reference game and allow its deliberate unhandled exception to occur.
14. Verify ZERO remains alive.
15. Verify a non-empty `.dmp` and package-specific crash JSON exist under `%LOCALAPPDATA%\ZERO\CrashReports\zero.system.reference`.
16. Run the installed `ZeroAcceptance.exe`.
17. Every acceptance check must print PASS and the process must exit 0.
18. Uninstall ZERO normally and verify player saves remain.

## Required acceptance checks

`ZeroAcceptance.exe` must prove:

- First Boot completion;
- hardened reference-package import with SHA-256 integrity inventory;
- package-specific Runtime session and SDK READY;
- Resume and achievement SDK contract;
- Resume persistence and round trip;
- canonical SQLite platform state;
- real minidump + package-specific crash diagnostics.

## Explicit non-claims

This gate does not certify:

- hostile-code sandboxing;
- public publisher signing or PKI;
- anti-cheat;
- Store commerce;
- cloud identity/social services;
- HDR or hardware certification;
- ARM64 or non-Windows platforms.

Only after the automated CI gate and the full real-machine acceptance journey pass may Runtime V4.1 be called RC-qualified.
