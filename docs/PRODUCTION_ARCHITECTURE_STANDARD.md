# ZERO Production Architecture Standard

This document defines the production baseline for ZERO Player. It applies to the shell, ZERO Core V5 / ProductionRuntime, package services, input, persistence, installer/update, social/store providers, captures, SDK integration, diagnostics, CI, release packaging, repository governance, signing, and future authoritative online services.

## Production rules

1. **Single authority per concern.** Navigation, package trust, persistence, paths, input arbitration, launch policy, parsing, UX state, release qualification, and online authority each have one authoritative contract. Duplicate switch tables, magic destination indexes, parallel state models, independent package JSON readers, or local substitutes for server-owned state are architectural defects.
2. **Truthful provider boundaries.** Disconnected services render disconnected states. ZERO never fabricates friends, Store products, entitlements, captures, identity, cloud state, or package trust.
3. **Fail closed for security and destructive actions.** Invalid manifests, malformed/duplicate JSON, integrity failures, invalid signatures, unknown trusted keys, unsafe paths, malformed setup state, missing release credentials/evidence, and destructive operations resolve to blocked/recovery states.
4. **Deterministic state machines.** Shell navigation, overlay lifecycle, input, First Boot, Captures, Friends, launch recovery, install/update, and release qualification use explicit states and deterministic transitions.
5. **Persistence is transactional and test-isolated.** Program replacement, package installation, First Boot, settings, canonical game state, and release metadata use atomic/rollback behavior. Persistent services support isolated acceptance paths rather than silently touching a developer/player profile.
6. **No title-specific platform architecture.** Games integrate through manifests, Runtime/SDK contracts, storage namespaces, Resume, achievements, and platform APIs. No commercial title defines ZERO behavior.
7. **No source file over 1,000 lines.** Files approaching the limit split by responsibility before exceeding it.
8. **Compiler warnings are release failures.** MSVC production builds use `/W4 /WX /permissive- /utf-8`.
9. **No unresolved production debt markers.** TODO/FIXME/HACK and explicit prototype/mock/placeholder-data markers fail the architecture gate.
10. **Every production subsystem requires executable acceptance evidence.** Contract, cross-system, hostile-input, persistence, fault-injection, and performance tests are required; real-hardware gates remain mandatory where CI cannot reproduce the environment.
11. **Release evidence is immutable for an exact commit.** Any source, shell, runtime, SDK, build, installer, manifest, workflow, or packaging change invalidates prior physical RC qualification.
12. **Production binaries are signed.** A production release must contain a valid Authenticode-signed shipping executable. Missing/invalid signing credentials or signatures fail closed. CI RC artifacts are never relabeled as production artifacts.
13. **Every release has supply-chain evidence.** Production and RC candidate bundles include an SPDX SBOM and SHA-256 inventory bound to the exact commit.
14. **CI dependencies are immutable.** External GitHub Actions used by authoritative workflows are pinned to exact commit SHAs, not mutable major-version tags.
15. **Online/commercial state is server-authoritative.** The Windows client is untrusted for identity, commerce, entitlements, friends, presence/join authorization, cloud state, publisher enrollment/revocation, sanctions, rank, anti-cheat verdicts, and multiplayer authority. If the authoritative service is unavailable or unverifiable, the feature fails closed/disconnected.
16. **Main-branch governance is mandatory.** The production repository must require PR review and the Windows Build and CodeQL status checks before changes reach `main`; direct bypass is a production-governance failure.

## Layer model

### Shell presentation layer
Owns rendering, focus, transitions, page composition, accessibility presentation, DPI-aware presentation, and user-visible recovery. It consumes state from services/contracts and does not duplicate business rules.

### Experience state layer
Owns deterministic state for Home/Library/Game Detail, Friends, Captures, Store, Settings, First Boot, Achievements, overlay, and launch recovery. State helpers remain testable without Direct2D/Win32 UI.

### Platform service layer
Owns GameRegistry, GameImportService, CaptureLibrary, identity, Friends/Store providers, ResumeStore, achievement/crash services, settings, and canonical database access. Services expose explicit success/failure results rather than UI side effects.

### Runtime/security layer
Owns ZERO Core V5 ProductionRuntime lifecycle, process containment, authenticated SDK IPC, package integrity, strict package metadata parsing, ECDSA P-256/SHA-256 publisher verification, trust policy, launch blocking, diagnostics, and crash containment.

### Online authority layer
Owns server-side authentication, entitlement issuance, commerce mutation, social graph, presence/join authorization, cloud state, publisher enrollment/revocation, rate limiting, abuse/fraud policy, and any future competitive/multiplayer authority. The client can request/display/cache state but cannot mint authoritative state. See `docs/SERVER_AUTHORITY_SECURITY.md`.

### Persistence layer
Owns durable local state and migration/compatibility behavior. SQLite is canonical for local platform state. Player data is separated from replaceable program files. Local state is never an online authorization source.

### Release engineering layer
Owns fatal-warning builds, CodeQL analysis, automated acceptance, architecture measurement, performance budgets, package manifests/hashes, dependency evidence, SPDX SBOM generation, Authenticode verification, installer/update verification, CI evidence, and physical qualification.

## Measurable release benchmarks

Every candidate produces `build/ArchitectureReports/production-architecture.json` and a Markdown summary. The architecture gate enforces:

- current documentation identifies ZERO Core V5 / ProductionRuntime;
- publisher trust documentation matches `ecdsa-p256-sha256`;
- manifest Registry and Import use the shared typed `PackageManifestParser`;
- signature and trusted-key metadata use `StrictJson`;
- duplicate JSON keys, malformed Unicode, excessive nesting, and trailing JSON data are rejected;
- MSVC production warnings are fatal;
- shell startup opts into Per-Monitor V2 DPI awareness;
- navigation and storage discovery each have one authority;
- no legacy parallel shell state remains;
- no source file exceeds 1,000 lines;
- no banned production-debt markers remain;
- at least 14 dedicated acceptance-contract sources exist;
- Release shell and all acceptance executables exist;
- shell binary remains below the 64 MiB release budget;
- acceptance binaries and shell are inventoried by bytes and SHA-256;
- authoritative GitHub Actions are SHA-pinned;
- the RC pipeline emits an SPDX SBOM;
- a separate production-release path requires exact-commit physical qualification plus valid Authenticode signing.

The automated M1 report records each gate duration, the slowest gate, and enforces a total 600,000 ms acceptance budget. `ZeroProductionBenchmarkAcceptance` additionally enforces bounded strict-JSON parsing and FULL-synchronous SQLite session-transaction performance in CI. These are regression budgets, not substitutes for hardware performance qualification.

## Required automated evidence

A production candidate passes, at minimum:

- Runtime/SDK IPC persistence;
- package integrity;
- publisher trust cryptography;
- package repair/re-import;
- strict JSON hostile-input rejection;
- hermetic canonical SQLite persistence and idempotency;
- local storage failure/fault injection;
- production core performance budgets;
- input arbitration/reconnect/hysteresis;
- shell/overlay UX state;
- permanent navigation contract;
- Friends contract;
- Captures contract;
- Home/Library/Game Detail contract;
- Store/Settings/First Boot contract;
- First Boot persisted-state recovery;
- installer/update/uninstall preservation;
- reference-package contract;
- RC payload verification;
- production architecture gate;
- CodeQL C++ security-and-quality analysis;
- deterministic release packaging and SHA-256 manifest generation;
- SPDX SBOM generation.

A **production** release additionally requires valid Authenticode verification and physical qualification evidence for the exact release commit.

## Physical qualification boundary

CI cannot certify real Windows 11 client behavior, real controller enumeration/reconnect, foreground recovery against arbitrary native games, reboot persistence, multi-monitor/DPI behavior on actual displays, complete interactive First Boot, or the legitimacy/possession of an external production signing identity. Those remain physical/operational release requirements for the exact candidate commit.

## Server-authoritative boundary

ZERO Core V5 / ProductionRuntime is the production local-console foundation. Current Store and Friends providers intentionally remain disconnected until a deployed authoritative service exists. A future online service cannot be considered production-ready merely because client UI/provider code exists. It must satisfy `docs/SERVER_AUTHORITY_SECURITY.md`, server-side authorization, replay/idempotency, rate-limit/abuse controls, secret/token handling, hostile-input testing, degraded-network behavior, and production monitoring.

## Deferred ecosystem capabilities

Production-grade ZERO Core V5 / ProductionRuntime does not imply completion of the online commercial ecosystem. Store commerce, cloud identity, social backend, DRM, anti-cheat, native video capture/encoding, public publisher CA/portal, and public SDK distribution are separate milestones and remain truthful disconnected/not-supported states until implemented and independently qualified.
