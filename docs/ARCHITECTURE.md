# ZERO Virtual Console Architecture

ZERO Virtual Console is a standalone, game-agnostic Windows platform. Games integrate through stable manifests, Runtime/SDK contracts, storage namespaces, Resume, achievements, and platform services; they never define the console architecture.

## Production architecture

```text
Windows 11 x64
   |
ZeroVirtualConsole.exe
   |
   +-- Production Shell
   |     Home / Library / Store / Friends / Captures / Settings
   |     Game Detail / Import / Achievements / Launch Recovery
   |     ShellUxState / ProductionUxContract / experience-state contracts
   |
   +-- Platform Services
   |     GameRegistry / GameImportService / CaptureLibrary
   |     Identity / Friends / Store providers
   |     Settings / First Boot / Resume / Achievements / Crash Reports
   |     Canonical SQLite state
   |
   +-- Runtime V4.1
   |     manifest validation
   |     package integrity + publisher trust policy
   |     Runtime/SDK session and READY lifecycle
   |     CreateProcessW(CREATE_SUSPENDED)
   |     Windows Job Object containment
   |     process/child lifecycle monitoring
   |     overlay state + foreground recovery
   |     crash containment and diagnostics
   |
   +-- Release Engineering
         acceptance executables
         production architecture metrics
         installer/update/uninstall verification
         deterministic release bundle + SHA-256 manifest
         physical RC qualification evidence
```

## Architectural rules

- Runtime, shell, provider, persistence, and security contracts must remain game-agnostic.
- Each concern has one authority. Duplicate navigation tables, trust policy, persistence rules, or parallel UX state machines are technical debt.
- Security-sensitive and destructive operations fail closed.
- Disconnected online services expose truthful disconnected states rather than mock data.
- Program binaries are replaceable; player data is durable and stored separately.
- Source modules stay below 1,000 lines and are split by responsibility before becoming monolithic.
- CI verifies deterministic contracts; physical hardware acceptance verifies behavior CI cannot reproduce.

The enforceable version of these rules lives in `docs/PRODUCTION_ARCHITECTURE_STANDARD.md` and `tools/verify-production-architecture.ps1`.

## Runtime V4.1 lifecycle

Runtime V4.1 validates package metadata and launch policy before process creation. It creates the native game suspended, applies the containment/session environment, assigns the game to a Windows Job Object, resumes execution, tracks Runtime/SDK READY and persistence events, monitors process outcome, and restores the ZERO shell after clean exit, user termination, startup failure, hang/crash paths, or explicit recovery.

Per-game runtime storage is isolated under `%LOCALAPPDATA%/ZERO`:

```text
Library/<installed-package>/
Saves/<package-id>/
Cache/<package-id>/
Temp/<package-id>/<session-id>/
Data/zero.db
CrashReports/<package-id>/
Captures/
Trust/Publishers/
```

## Package security and trust

Package integrity and publisher identity are separate controls.

- `zero.integrity.sha256` inventories installed payload files and SHA-256 digests.
- Runtime V4.1 verifies package integrity before launch.
- Optional `zero.signature.json` signs the exact integrity-manifest bytes.
- The supported publisher signature algorithm is `ecdsa-p256-sha256`.
- Windows CNG verifies signatures against locally trusted P-256 public keys.
- Local/sideloaded games may be allowed unsigned under `AllowLocalUnsigned` while malformed or invalid signature envelopes fail closed.
- Store/managed distribution can adopt `RequireTrustedPublisher` without changing Runtime launch architecture.

This is a local trusted-key foundation, not a public publisher CA, revocation service, Store signing service, DRM, or entitlement backend.

## Shell and experience architecture

Permanent navigation is defined only by `ProductionUxContract`:

1. Home
2. Library
3. Store
4. Friends
5. Captures
6. Settings

Game Detail, Import, and Achievements are secondary pages.

State-heavy experiences use deterministic, independently testable contracts rather than encoding behavior only in Direct2D/Win32 rendering code. Current contracts cover shell/overlay state, navigation, Friends, Captures, Home/Library/Game Detail, Store/Settings/First Boot, input, launch recovery, package trust, install/update, and release qualification.

## Persistence model

Durable state includes installed games, canonical sessions/statistics, Resume, achievements, settings, First Boot state, saves, captures, trust keys, and diagnostics. Program updates never intentionally replace player data. Install/update and package import paths use staging/rollback or atomic-finalization behavior.

## Observability and release evidence

Production candidates generate machine-readable evidence rather than relying on subjective review:

- `build/ArchitectureReports/production-architecture.json`
- `build/AcceptanceReports/m1-automated-acceptance.json`
- `build/QualificationEvidence/rc-qualification.json`
- `build/ReleaseBundle/release-manifest.json`

The architecture report records exact commit, source-file/line counts, largest source file, acceptance-contract count, built acceptance binaries, binary sizes/hashes, and architecture-policy violations.

## Qualification boundary

Automated acceptance is necessary but insufficient. Final RC qualification still requires the real Windows 11 x64 + physical XInput controller journey documented in `docs/RUNTIME_V4_1_RC_GATE.md`. Any source/build/runtime/shell/SDK/installer/manifest change after physical qualification invalidates the evidence for that candidate.
