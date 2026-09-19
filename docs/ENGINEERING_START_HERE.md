# ZERO Player — Engineering Start Here

This document is the first stop for engineers working on ZERO Player.

ZERO Player is one Windows gaming platform product. It is not a collection of independent screens and it is not tied to a single game. The product goal is a console-grade experience with the immediacy of a game player, the management depth of a professional game launcher, and a premium store/discovery experience — while remaining unmistakably ZERO.

## Read these first

1. `docs/ZERO_PLAYER_UI_LOCK.md` — locked product/UX direction.
2. `docs/ZERO_CORE_ARCHITECTURE_V5.md` — authority and runtime boundaries.
3. `docs/PRODUCT_ARCHITECTURE.md` — current application/module structure.
4. `docs/PRODUCTION_CLOSURE_POLICY.md` — release discipline and blocker rules.
5. `SECURITY.md` — security invariants.

## The rule that matters most

There is one source of truth for every concern.

- The shell owns navigation and presentation state.
- Platform/domain authorities own business truth.
- Runtime owns the active game session.
- Server-backed services own online/commercial truth.
- Views render state and emit commands; they do not invent authority.

Do not add a second cache, second navigation state, second entitlement source, second download queue, or page-local copy of authoritative state merely because it is convenient.

## Composition root

`ApplicationServices` is the production composition root. Long-lived services are created there and injected into the application shell.

When a disconnected adapter becomes a real backend integration, replace it in the composition root. Do not wire HTTP/service logic directly into Home, Store, Friends, Profile, Downloads, Cloud Play, or Settings.

## Feature boundaries

The product is organized conceptually into these feature modules:

- Shell — navigation, focus, transitions, top bar, overlay, global commands.
- Home — projections of library/runtime/download/capture/social state.
- Discover — catalog/local discovery projection.
- Store — catalog, offers, wishlist, checkout presentation.
- Library — installed/owned content management and Game Detail.
- Downloads — transfers, verification, staging, install/update state.
- Identity — ZERO ID/local identity projection and account session state.
- Social — friends, presence, parties, invites, join authorization.
- Achievements — definitions, progress, unlocks, score, profile projection.
- Cloud — cloud saves and cloud gaming allocation/stream state.
- Capture — screenshots/clips/gallery/viewer/export lifecycle.
- Devices — local controller/PC state and authoritative remote devices.
- Notifications — unified local/online event projection.
- Settings — persisted user preferences and diagnostics.
- Runtime — launch policy, process supervision, IPC, Resume, crash recovery.
- Package Platform — import/acquire/validate/trust/stage/commit/register lifecycle.

Feature modules may depend on domain/service interfaces. They may not own backend truth.

## Change workflow

For every change:

1. Name the authority being changed.
2. Confirm no competing authority is introduced.
3. Add or update the boundary acceptance test.
4. Keep disconnected behavior truthful and fail-closed.
5. Run the exact-head Windows and CodeQL gates.
6. For visible work, verify the locked ZERO UI at supported resolutions/DPI.

## Where to add code

- New backend/service adapter: service/platform layer, then register in `ApplicationServices`.
- New domain rule: `include/v5` + `src/v5`, with an acceptance target.
- New visible experience: feature state/projection first, renderer second.
- New navigation destination: ShellKernel/production shell contract first.
- New package/install behavior: Package Platform only.
- New runtime behavior: Runtime Core only.

If you are about to add business logic directly to `AppPages.cpp` or `main.cpp`, stop and determine which feature/domain owns that behavior.

## Definition of done

A feature is complete only when it has:

- one authority;
- explicit service/domain boundary;
- testable state/commands;
- production UX integration;
- mouse, keyboard and controller parity where applicable;
- truthful disconnected/failure states;
- automated acceptance;
- no known production blocker in its scope.

Compilation is not completion.
