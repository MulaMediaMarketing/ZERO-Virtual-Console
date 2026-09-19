# ZERO Core Architecture V5 — Production Cutover Status

This document records the live ZERO Player production-authority state. It is a gate document, not a production-completion claim.

## Runtime production authority

The ZERO Player consumer executable owns runtime launch through `zero::ProductionRuntime` (`include/v5/ProductionRuntime.h`, `src/v5/ProductionRuntime.cpp`). `App` no longer owns or imports Runtime V3/V4 authority.

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

`App` imports packages through `v5::ImportCoordinator` and reads Resume state through `ProductionRuntime`; direct App ownership of `GameImportService` and `ResumeStore` has been removed.

## Live shell production authority

`v5::ShellKernel` is the active-page and top-level-navigation authority for the live ZERO Player shell. `ProductionShellIntegration` is the composition/translation boundary between Win32 presentation and `ShellKernel`; it is not a second authority.

The twelve permanent destinations remain:

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

Navigation is fail-closed and deterministic. Contextual history is owned by `ShellKernel`; failed activation cannot publish a new navigation revision.

## Legacy runtime status

`RuntimeV3.cpp` and `RuntimeV4.cpp` remain in source history for compatibility archaeology but are excluded from the `ZeroVirtualConsole` consumer target through `cmake/ZeroV5.cmake`. No V5 source may import either legacy runtime authority.

Protocol-4 package bootstrap compatibility remains supported where needed. Protocol compatibility does not restore Runtime V4 authority.

## Online-service authority boundaries

ZERO ID, Catalog/Entitlements, Commerce, Social, ZERO Cloud, remote Devices and related online domains remain fail-closed unless backed by real ZERO services. Missing providers may not be replaced by synthetic production data.

## Verified repository state

At the audit baseline `816b4ae834fb7d047612fdf8d5f8dc3ebd38cdd5`:

- Windows Build #329 passed.
- CodeQL C++ #198 passed.
- `main` governance is active with required review, strict required `build`/`analyze` checks, stale-review dismissal, last-push approval, thread resolution and force-push/deletion protection.
- ShellKernel production ownership, V5 import ownership and V5 persistence projections are enforced by repository gates.

## Remaining production blockers

The release remains blocked until all applicable items below are cleared:

- final whole-product UX and physical visual qualification across supported resolutions and DPI;
- placeholder preview/artwork consistency and packaging cleanup;
- resolution of outstanding PR review threads and required human approval;
- production Store/catalog/commerce/entitlement provider deployment and credentials;
- production Friends/social/presence/invite provider deployment and credentials;
- production ZERO Cloud deployment and credentials;
- production remote content/CDN delivery integration and credentials;
- online ZERO ID/profile and remote-device service deployment where required;
- production Authenticode certificate/secrets and successful signed release workflow;
- exact-commit Windows 11 physical qualification;
- final end-to-end validation on the exact signed production artifact.

## Acceptance

A production cutover/release candidate is acceptable only when:

- all V5 acceptance gates pass;
- `verify-v5-cutover.ps1` passes;
- production architecture and closure gates pass;
- Windows Build passes on the exact candidate;
- CodeQL passes on the exact candidate;
- no legacy runtime authority is linked into `ZeroVirtualConsole`;
- `App` owns `ProductionRuntime` and `v5::ImportCoordinator` projections rather than legacy authorities;
- `ShellKernel` is the single live navigation authority;
- required review/governance gates are satisfied;
- physical qualification and production signing are complete for the exact candidate;
- no known production blocker remains.

Until then the valid state is **BLOCKED — specific named gate failure**.
