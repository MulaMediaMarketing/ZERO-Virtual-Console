# ZERO Core Architecture V5 — Final Cutover Status

This document records the production-authority transition from the historical Runtime V3/V4 lineage to ZERO Core Architecture V5.

## Production authority

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
10. domain-repository persistence

## Legacy runtime status

`RuntimeV3.cpp` and `RuntimeV4.cpp` are excluded from the `ZeroVirtualConsole` consumer target. They remain in repository history/source during this cutover only for auditability and compatibility archaeology; they are not production runtime authorities.

No V5 source file may include `RuntimeV3.h` or `RuntimeV4.h`. The shell may not reference either legacy runtime type. `tools/verify-v5-cutover.ps1` enforces these rules in Windows CI.

## SDK compatibility

The V5 native host currently preserves protocol-4 bootstrap compatibility for existing ZERO SDK game payloads. Protocol compatibility does not imply Runtime V4 authority: process ownership, session identity, capability grants, crash handling, Resume handling, and shell runtime ownership are V5.

## Authority boundaries that remain disconnected

The V5 client domains for Zero ID, Catalog/Entitlements, Commerce, Social, Zero Cloud, Devices, and other online services remain fail-closed unless backed by real ZERO services. This cutover does not fabricate those backends.

## Acceptance

A V5 cutover is acceptable only when:

- all `ZeroV5*Acceptance.exe` gates pass;
- `verify-v5-cutover.ps1` passes;
- Windows Build passes on the exact PR head;
- CodeQL C++ passes on the exact PR head;
- no legacy runtime implementation is linked into `ZeroVirtualConsole`;
- `App` owns `ProductionRuntime` rather than a legacy runtime coordinator.

Until those gates are green on the final cutover PR head, the migration is not considered complete.
