# ZERO Core Architecture V5 — Production Cutover Status

This document records the production-authority transition from the historical Runtime V3/V4 lineage and legacy shell navigation to ZERO Core Architecture V5. It is a gate document, not a completion claim.

## Runtime production authority

The ZERO Player consumer executable owns runtime launch through `zero::ProductionRuntime` (`include/v5/ProductionRuntime.h`, `src/v5/ProductionRuntime.cpp`). `App` no longer owns or imports `RuntimeV3` or `RuntimeV4`.

The authoritative launch path is:

1. package integrity and trust validation
2. V5 launch/session identity allocation
3. `RuntimeAuthority` launch policy validation
4. capability grant
5. `NativeRuntimeProcessHost`
6. contained native process creation
7. capability-scoped secure IPC
8. READY / heartbeat lifecycle supervision
9. V5 crash supervision and Resume coordination
10. V5 session-repository persistence

Compatibility persistence adapters/stores still exist around portions of Resume, achievements, and platform-state access. Their remaining ownership cleanup is a production blocker and is not represented here as completed.

## Live shell production authority

`v5::ShellKernel` is the active-page and top-level-navigation authority for the live ZERO Player shell.

The live shell contract has exactly twelve permanent destinations, in this order:

1. Home
2. Discover
3. Store
4. Library
5. Cloud Play
6. Downloads
7. Friends
8. Achievements
9. Capture
10. Profile
11. Devices
12. Settings

`ProductionShellIntegration` is the composition/translation boundary between the Win32 presentation layer and `ShellKernel`; it is not a second navigation authority. App page/navigation members are projections over the V5 shell rather than independent stored state.

Shell navigation requirements are fail-closed and deterministic:

- unavailable destinations cannot silently route to the wrong page;
- top-level traversal policy is owned by `ShellKernel`;
- controller activation must succeed before a navigation state change is published;
- contextual pages retain their permanent top-level origin;
- failed activation leaves active page, parent, and navigation revision unchanged;
- the live renderer uses width-aware navigation layout for all twelve permanent destinations.

The shell cutover is not considered accepted until exact-head CI and all V5 acceptance gates are green.

## Legacy runtime status

`RuntimeV3.cpp` and `RuntimeV4.cpp` are excluded from the `ZeroVirtualConsole` consumer target. They remain in repository history/source during this cutover only for auditability and compatibility archaeology; they are not production runtime authorities.

No V5 source file may include `RuntimeV3.h` or `RuntimeV4.h`. The shell may not reference either legacy runtime type. `tools/verify-v5-cutover.ps1` enforces these rules in Windows CI.

## SDK compatibility

The V5 native host currently preserves protocol-4 bootstrap compatibility for existing ZERO SDK game payloads. Protocol compatibility does not imply Runtime V4 authority: process ownership, session identity, capability grants, crash handling, Resume handling, and shell runtime ownership are V5.

## Authority boundaries that remain disconnected

The V5 client domains for Zero ID, Catalog/Entitlements, Commerce, Social, Zero Cloud, Devices, and other online services remain fail-closed unless backed by real ZERO services. This cutover does not fabricate those backends.

A permanent destination may exist in the V5 shell contract while its production experience or service remains unavailable. Missing service infrastructure is never replaced by sample catalog data, synthetic users, local entitlement authority, or fabricated cloud state.

## Known production blockers after shell cutover

The release remains blocked until all of these are cleared:

- persistence and import ownership cleanup, including direct App `ResumeStore` and `GameImportService` ownership;
- remaining visible page integrations and truthful disconnected-state rendering;
- final whole-product UX and acceptance qualification;
- protected `main` with required review, Windows Build, and CodeQL gates;
- production Authenticode signing;
- exact-commit physical Windows qualification.

## Acceptance

A V5 production cutover is acceptable only when:

- all `ZeroV5*Acceptance.exe` gates pass;
- `verify-v5-cutover.ps1` passes;
- the production architecture gate passes;
- Windows Build passes on the exact PR head;
- CodeQL C++ passes on the exact PR head;
- no legacy runtime implementation is linked into `ZeroVirtualConsole`;
- `App` owns `ProductionRuntime` rather than a legacy runtime coordinator;
- `ShellKernel` is the single live navigation authority;
- no known production blocker remains.

Until those gates are green on the exact release-candidate commit, the only valid release state is **BLOCKED — specific named gate failure**. The migration must not be described as production-complete before that point.
