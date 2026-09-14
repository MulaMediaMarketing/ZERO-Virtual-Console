# ZERO Production Architecture Standard

This document defines the production baseline for ZERO Virtual Console. It applies to the shell, Runtime V4.1, package services, input, persistence, installer/update, social/store providers, captures, SDK integration, diagnostics, CI, and release packaging.

## Production rules

1. **Single authority per concern.** Navigation, package trust, persistence, paths, input arbitration, launch policy, and UX state each have one authoritative contract. Duplicate switch tables, magic destination indexes, or parallel state models are architectural defects.
2. **Truthful provider boundaries.** Disconnected services must render disconnected states. ZERO must not fabricate friends, Store products, entitlements, captures, identity, cloud state, or package trust.
3. **Fail closed for security and destructive actions.** Invalid manifests, integrity failures, invalid signatures, unknown trusted keys, unsafe paths, malformed setup state, and delete actions must resolve to blocked/recovery states rather than optimistic execution.
4. **Deterministic state machines.** Shell navigation, overlay lifecycle, input, First Boot, Captures, Friends, launch recovery, install/update, and release qualification must have explicit states and deterministic transitions.
5. **Persistence is transactional.** Program replacement, package installation, First Boot, settings, canonical game state, and release metadata must be written atomically or with rollback/recovery semantics.
6. **No title-specific platform architecture.** Games integrate through manifests, Runtime/SDK contracts, storage namespaces, Resume, achievements, and platform APIs. No commercial title may define ZERO behavior.
7. **No source file over 1,000 lines.** Files approaching the limit must be split by responsibility before exceeding it.
8. **No unresolved production debt markers.** TODO/FIXME/HACK and explicit prototype/mock/placeholder-data markers fail the production architecture gate.
9. **Every production subsystem requires acceptance evidence.** Unit-like contract tests are necessary but not sufficient; cross-system acceptance and real-hardware gates are required where CI cannot reproduce the environment.
10. **Release evidence is immutable for an exact commit.** Any source, shell, runtime, SDK, build, installer, manifest, or packaging change invalidates prior physical RC qualification.

## Layer model

### Shell presentation layer
Owns rendering, focus, transitions, page composition, accessibility presentation, and user-visible recovery. It consumes state from services/contracts and must not duplicate business rules.

### Experience state layer
Owns deterministic state for Home/Library/Game Detail, Friends, Captures, Store, Settings, First Boot, Achievements, overlay, and launch recovery. State helpers must be testable without Direct2D/Win32 UI.

### Platform service layer
Owns GameRegistry, GameImportService, CaptureLibrary, identity, Friends/Store providers, ResumeStore, achievement/crash services, settings, and canonical database access. Services expose explicit success/failure results rather than relying on UI side effects.

### Runtime/security layer
Owns Runtime V4.1 lifecycle, process containment, SDK IPC, package integrity, ECDSA P-256/SHA-256 publisher verification, trust policy, launch blocking, diagnostics, and crash containment.

### Persistence layer
Owns durable local state and migration/compatibility behavior. Player data is separated from replaceable program files.

### Release engineering layer
Owns automated acceptance, architecture measurement, package manifests/hashes, installer/update verification, CI evidence, and physical qualification.

## Measurable release benchmarks

Every candidate must produce `build/ArchitectureReports/production-architecture.json` and Markdown summary. The architecture gate currently enforces:

- current public documentation identifies Runtime V4.1;
- publisher trust documentation matches `ecdsa-p256-sha256`;
- no duplicate `pageForNavIndex` mapping remains in the shell;
- no source file exceeds 1,000 lines;
- no banned production-debt markers remain;
- at least 10 dedicated acceptance-contract sources exist;
- Release shell binary exists;
- built acceptance binaries are inventoried with byte sizes and SHA-256 hashes.

The report also records total source files, total source lines, largest source file, acceptance-contract count, acceptance-binary count, exact commit, and policy violations. These metrics create a benchmark that can be compared between future releases rather than relying on subjective assessment.

## Required automated evidence

A production candidate must pass, at minimum:

- Runtime/SDK IPC persistence;
- package integrity;
- publisher trust cryptography;
- package repair/re-import;
- input arbitration/reconnect/hysteresis;
- shell/overlay UX state;
- permanent navigation contract;
- Friends contract;
- Captures contract;
- Home/Library/Game Detail contract;
- Store/Settings/First Boot contract;
- installer/update/uninstall preservation;
- reference-package contract;
- RC payload verification;
- production architecture gate;
- deterministic release packaging and SHA-256 manifest generation.

## Physical qualification boundary

CI cannot certify real Windows 11 client behavior, real controller enumeration/reconnect, foreground recovery against native games, reboot persistence, or complete interactive First Boot. Those remain physical RC requirements for the exact candidate commit.

## Deferred ecosystem capabilities

Production-grade Runtime V4.1 does not imply completion of the online commercial ecosystem. Store commerce, cloud identity, social backend, DRM, anti-cheat, native video capture/encoding, public publisher CA/portal, and public SDK distribution are separate milestones and must remain truthful disconnected/not-supported states until implemented.
