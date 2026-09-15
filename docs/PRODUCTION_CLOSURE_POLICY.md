# ZERO Player Production Closure Policy

ZERO Player is in production-closure mode. This phase exists to finish one integrated, releasable executable. It is not an architecture-expansion phase.

## Mandatory PR rule

Every pull request must resolve at least one concrete item from the active production-blockers list.

A PR is not valid closure work if it only:
- adds another architectural layer;
- introduces speculative future infrastructure;
- creates a parallel authority instead of removing one;
- adds mock/fake backend behavior;
- moves code without closing a defined release blocker;
- claims "production-ready" while a known blocker remains unresolved.

Each PR description must name the blocker it resolves, the acceptance evidence required, and any blocker that remains afterward.

No PR may be merged while it introduces a new known failure, weakens an acceptance gate, hides a warning, skips a failing test, or leaves its targeted blocker only partially resolved.

## Active production blocker queue

The closure order is:

1. **Live ShellKernel ownership cutover**
   - `ShellKernel` becomes the single live navigation authority.
   - The renderer consumes authoritative shell/page snapshots.
   - Legacy `ProductionUxPage` / `page_` navigation authority is removed or demoted so it cannot compete with V5.

2. **Persistence and import ownership cleanup**
   - App-level direct `ResumeStore` ownership is removed.
   - Direct App orchestration of `GameImportService` is replaced by the V5 package authority path.
   - Persistence access is routed through the authoritative repository/coordinator boundaries.
   - Compatibility stores may not remain as a second source of truth.

3. **Playtime finalization and checkout overflow fixes**
   - Completed zero-second sessions remain finalized at zero and can never start growing again.
   - Checkout subtotal/tax arithmetic is explicitly overflow-safe before addition.
   - Regression acceptance covers both defects.

4. **Remaining visible page integrations**
   - Discover
   - Downloads
   - Profile
   - Devices
   - Wishlist
   - Checkout
   - Notifications
   - Cloud Play
   - Each page must be fully integrated with the live shell/renderer.
   - Backend-dependent pages must show truthful, polished disconnected/unavailable states when no authoritative service exists. No mock service data.

5. **Final whole-product UX and acceptance testing**
   - full permanent navigation and contextual Back behavior;
   - controller, keyboard, and mouse paths;
   - launch, Resume, overlay, crash, recovery, achievements, captures, settings, import, persistence, offline/disconnected states;
   - DPI/scaling/accessibility and focus behavior;
   - no dead buttons, stale state, blank pages, duplicate authorities, programmer UI, or unhandled recovery states;
   - all automated acceptance and exact-head Windows Build + CodeQL green.

6. **Release governance and qualification**
   - protect `main`;
   - require PR review;
   - require Windows Build and CodeQL status checks;
   - produce the exact release candidate with valid Authenticode signing;
   - complete physical Windows qualification against that exact commit;
   - any release-affecting change after qualification invalidates the qualification evidence.

## Closure discipline

The only acceptable end states are:

1. **SIGNED + QUALIFIED RELEASE CANDIDATE** — all software, CI, governance, signing, and physical-qualification gates are green for one exact commit; or
2. **BLOCKED** — a specific named gate is failing, with no ambiguity about what prevents release.

Terms such as "almost production-ready", "basically complete", "production-ready except", or similar ambiguous completion language are not acceptable release states.

## PR acceptance contract

Before merge, every PR must prove:

- the named blocker is actually removed, not wrapped or deferred;
- affected acceptance tests pass;
- the full V5 acceptance surface remains green;
- Windows Build passes on the exact head;
- CodeQL passes on the exact head;
- no unresolved warnings, review threads, security concerns, or flaky behavior remain;
- no backend authority is fabricated;
- no new duplicate authority is introduced;
- the production-blocker list is updated truthfully.

If any condition is not met, advancement stops at that PR.

## Current phase goal

Finish the existing V5 architecture as one coherent ZERO Player executable. Do not expand the architecture until the production blocker list is empty and the release candidate is signed and physically qualified.
