# ZERO Core Architecture V5 Migration Status

This document records authority ownership during the V4.1 -> V5 migration. A subsystem is not considered migrated because a wrapper exists.

## Status vocabulary

- `NOT STARTED`: V4.1 remains the only implementation.
- `DUAL RUN`: V5 owns the public boundary but still delegates part of the implementation to a V4.1 component.
- `V5 AUTHORITATIVE`: production callers use the V5 authority and the old authority is no longer a decision-maker.
- `V4 REMOVED`: obsolete V4 implementation has been deleted after acceptance and compatibility requirements are satisfied.

## Current status

| Concern | Status | Current authority |
| --- | --- | --- |
| Content / entitlement model | V5 AUTHORITATIVE | `zero::v5::ContentModel` |
| Launch decision semantics | V5 AUTHORITATIVE | `zero::v5::PlatformKernel` |
| Online entitlement source | NOT STARTED | No production ZERO service is deployed; managed content therefore remains fail-closed |
| Package install/repair entry point | DUAL RUN | `ProductionPackagePlatform` owns parse/trust/final-integrity policy and delegates filesystem transaction mechanics to `GameImportService` |
| Package integrity | DUAL RUN | Existing hardened `PackageSecurity` / `PackageIntegrityVerifier` are reused by V5 |
| Publisher trust | DUAL RUN | Existing hardened trust provider is reused by V5; public publisher authority/revocation service is not deployed |
| Runtime Core | DUAL RUN | V5 capability/session model exists; process/runtime migration is not complete |
| Shell controllers | NOT STARTED | V4.1 `App.*` remains the presentation/orchestration path |
| Zero ID service | NOT STARTED | No production service deployed |
| Catalog / Discover backend | NOT STARTED | No production service deployed |
| Commerce / Store authority | NOT STARTED | No production service deployed |
| Friends / presence authority | NOT STARTED | No production service deployed |
| Cloud saves | NOT STARTED | No production service deployed |
| Zero Cloud streaming authority | NOT STARTED | No production service deployed |

## Package migration invariant

All V5 package install and repair callers must enter through `ProductionPackagePlatform`.

The V5 package authority performs:

`request validation -> manifest parse -> publisher trust policy -> transactional stage/commit -> installed integrity verification -> installed identity/version verification -> ready`

Managed catalog packages use `RequireTrustedPublisher`. `AllowLocalUnsigned` is reserved for explicit local/imported content and never upgrades a package into a managed ZERO entitlement.

## Launch authority invariant

A managed catalog title requires an entitlement whose `AuthoritySource` is `ZeroService`. Local cached state cannot satisfy this requirement.

An explicitly local package can require `AuthoritySource::LocalPackage`. The two authorities are intentionally non-interchangeable.

The kernel returns typed launch failures so presentation code cannot infer or invent authorization state.
