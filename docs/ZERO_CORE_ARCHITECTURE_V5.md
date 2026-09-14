# ZERO Core Architecture V5

ZERO Core Architecture V5 is the replacement architecture for ZERO Player. It is not a compatibility rename of Runtime V4.1 and it must not inherit monolithic application orchestration as a design pattern.

## Product model

ZERO Player is a game platform client for Zero Originals, premium games, free-to-play games, community experiences, local games, cloud games, captures, achievements, social presence, devices, and future ZERO hardware.

The platform is engine-agnostic. Unreal, Godot, native C++, Web, ZERO 2D, streaming, and future runtimes integrate through the same package and launch contracts.

## Core boundaries

### Application Shell

The shell owns presentation, navigation, focus, transitions, controller-first UX, and composition. It does not authorize launches, mint entitlements, validate packages, own commerce truth, or act as the source of social/cloud authority.

Primary shell domains:

- Home
- Discover
- Store
- Library
- Cloud Play
- Downloads
- Friends / Party
- Achievements
- Capture
- Profile
- Notifications
- Devices
- Settings

Wishlist is a Store sub-domain. Checkout is transactional and contextual. Game Detail is contextual and not a permanent top-level destination.

### Platform Kernel

The Platform Kernel coordinates application-level services through explicit interfaces. It must not depend on UI page implementation details.

Initial authorities:

- Identity
- Catalog
- Entitlements
- Launch Authority
- Package Platform
- Library
- Downloads / Updates
- Cloud Saves
- Achievements
- Social / Presence / Party
- Capture
- Devices
- Diagnostics

A launch request is legal only when an authenticated account session, catalog item, authoritative entitlement, and launch authority all agree.

### Runtime Core

Runtime Core owns one game session at a time through explicit launch policy, capability granting, process supervision, secure IPC, crash isolation, Resume, input, overlay, and lifecycle handling.

Runtime capabilities are granted per session. Authentication to the runtime channel does not automatically grant every platform API.

### Package Platform

All package installation and update flows use one lifecycle:

`Acquire -> Validate -> Trust -> Stage -> Commit -> Register -> Ready`

No Store, Library, Community, updater, local import, or creator feature may bypass this pipeline.

### Authority Platform boundary

The Windows client is not authoritative for online/commercial state.

Server-owned domains include:

- Zero ID authentication and account sessions
- catalog publication state
- prices and commerce
- entitlements and managed launch authorization
- friend graph, blocks, presence, invites, parties, and join authorization
- cloud saves / cloud Resume
- global achievements / score when configured as authoritative
- publisher enrollment and revocation
- moderation, sanctions, fraud, rate limiting, abuse controls
- cloud gaming allocation
- competitive rank and anti-cheat verdicts

If an authoritative service is unavailable or unverifiable, the corresponding feature fails closed or truthfully disconnects.

## Content model

Content classes are first-class and are not inferred from price:

- ZeroOriginal
- Premium
- FreeToPlay
- Community
- Demo
- DLC
- Expansion
- Experience

Entitlement classes are also first-class:

- Free
- Purchased
- Subscription
- Promotional
- Trial
- CreatorGrant
- Bundle
- TemporaryAccess

`price == 0` is never sufficient evidence that content may launch.

## Runtime types

V5 supports a common launch description for:

- NativeWin32
- Unreal
- Godot
- Web
- Zero2D
- Streaming
- FutureRuntime

Runtime implementation details stay behind the Runtime Core and are not exposed as product UX concepts.

## Migration rule

V4.1 remains the qualified local-console baseline until V5 replaces each authority with tested production implementations. New V5 work may reuse hardened V4.1 components such as StrictJson, package integrity, CNG publisher verification, SQLite, input code, and release tooling, but only behind V5 contracts.

No V5 subsystem is considered migrated merely because a wrapper calls a V4.1 class. Migration requires:

1. one declared V5 authority;
2. no parallel legacy authority for the migrated concern;
3. acceptance tests at the V5 boundary;
4. production architecture gate coverage;
5. exact-SHA Windows and CodeQL success.

## First implementation milestone: V5 Core Foundation

The first milestone establishes executable contracts for:

- engine-agnostic content and entitlement model;
- Platform Kernel launch request authority;
- single Package Pipeline;
- Runtime Core session and capability boundaries;
- fail-closed authoritative entitlement requirement;
- build-time V5 acceptance target.

This milestone does not claim the V5 shell, backend, commerce service, social service, cloud gaming backend, or creator platform are implemented.
