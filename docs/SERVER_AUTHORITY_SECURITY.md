# ZERO Server Authority and Exploit-Resistance Standard

ZERO Virtual Console is a native Windows client. A client process runs on hardware controlled by the player and is therefore never a trusted authority for online or commercial state.

## Non-negotiable authority rule

The ZERO client may request, display, cache, and optimistically animate server-owned state. It must never mint or finalize authoritative state for:

- Zero ID account identity or authentication;
- Store catalog authenticity, prices, purchases, refunds, or ownership;
- entitlements and managed-game launch authorization;
- Friends graph, requests, blocks, privacy, presence, invites, or join authorization;
- cloud saves, cloud Resume, achievements intended to be globally authoritative, or cross-device state;
- publisher enrollment, certificate/key issuance, revocation, or managed package trust;
- sanctions, abuse controls, parental controls, age/region policy, or commerce policy;
- competitive matchmaking/rank, anti-cheat verdicts, or multiplayer game state.

If the corresponding authoritative service is unavailable or its response cannot be authenticated and validated, ZERO fails closed for the authoritative operation and exposes a truthful disconnected/retry state.

## Client trust model

The Windows client, local SQLite database, local configuration, local clock, command line, environment variables, filesystem, registry, IPC peers outside ZERO's authenticated runtime channel, and user-supplied package metadata are attacker-controlled inputs for online-security purposes.

A future ZERO service response must be accepted only after all of the following are true:

1. TLS certificate validation succeeds using the operating-system trust store and current policy.
2. The request is authenticated to a server-issued session bound to the Zero ID account/device session.
3. The response is schema/version validated with strict size and depth limits.
4. Server-issued identifiers and authorization decisions are treated as opaque values, never reconstructed from local input.
5. Replay-sensitive operations carry server nonce/idempotency/version information.
6. The server enforces authorization independently of any UI state or client-provided role/ownership field.
7. Commerce and entitlement mutation is idempotent and auditable server-side.
8. Local cached state is advisory and cannot elevate privilege when the server disagrees or is unavailable.

## Exploit-resistance rules

- Never trust client-provided prices, ownership, roles, friend state, rank, currency, inventory, or security verdicts.
- Never grant an entitlement because a local file/database field says `Owned`.
- Never authorize a social join solely from local presence data.
- Never deserialize unbounded or duplicate-key JSON from a service.
- Never execute a path, command line, URL handler, or package payload merely because a remote field supplied it; pass through the same local validation/trust boundary used for installed packages.
- Never place reusable service secrets, publisher private keys, signing certificates, or backend credentials in the client binary or repository.
- Authentication/session tokens must be short-lived, scope-limited, revocable, protected using Windows credential facilities where persistence is unavoidable, and excluded from diagnostics.
- Sensitive server decisions must be logged with correlation IDs without logging credentials or raw tokens.
- Rate limiting, abuse prevention, fraud controls, entitlement issuance, and multiplayer authority are server responsibilities.

## Current production behavior

Runtime V4.1 is a production local-console foundation. The current Store and Friends implementations are deliberately disconnected providers. That is the correct secure behavior until real authoritative services exist. ZERO must not replace these providers with local mock ownership, fake catalog data, fake friends, or client-authoritative mutations in a production build.

## Release requirement

A future PR that enables an online authority surface must include, before merge:

- service threat model;
- authenticated protocol/schema contract;
- server-side authorization rules;
- replay/idempotency design;
- rate-limit/abuse design;
- client failure/offline behavior;
- secret/token storage design;
- integration, hostile-input, replay, authorization-bypass, and degraded-network acceptance tests;
- production monitoring and incident rollback procedure.

Until those artifacts and a deployed service exist, the feature remains disconnected and is not represented as production-complete.
