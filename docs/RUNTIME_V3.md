# ZERO Runtime V3

Runtime V3 extends the existing game-agnostic Runtime V2 process host with authenticated local SDK communication, READY state, Resume metadata, playtime tracking, and overlay focus signaling.

## Scope

Runtime V3 is a platform service. It contains no WanderTales-specific code, IDs, filenames, logic, or assumptions. Every registered game uses the same manifest/session/SDK contract.

## Launch flow

```text
ZERO Shell
  -> Game manifest
  -> Runtime V3 coordinator
  -> Runtime V2 native process host
  -> Job Object containment
  -> Game.exe
  -> Zero SDK bootstrap
  -> authenticated named-pipe handshake
  -> READY
  -> Running session
```

## Authenticated SDK IPC

The host generates a cryptographically random 256-bit session token using Windows CNG (`BCryptGenRandom`). The token is not placed on the game command line.

Runtime V3 creates a session-scoped named pipe and a bootstrap file inside the already isolated session temp directory. A ZERO-aware game uses the SDK to read that bootstrap contract and authenticate.

Handshake:

```text
Game -> HELLO <token> <package-id> 3
ZERO -> WELCOME 3
Game -> READY
ZERO -> ACK READY
```

Package identity is checked against the Runtime-owned active session. A client cannot authenticate as an unrelated package merely by changing a manifest field after launch.

## READY semantics

Runtime V3 does not treat process creation as game readiness. Playtime begins after the SDK reports READY.

This separates:

- process created
- runtime connected
- game actually ready for the player

## Resume metadata

A title may publish game-authored logical Resume state through the SDK:

```text
activity_id
display_label
payload
```

The runtime binds that state to the active package identity and persists it under:

```text
%LOCALAPPDATA%\ZERO\Resume\<package-id>.json
```

The game does not get to choose another package namespace.

Runtime V3 Resume is logical Resume, not arbitrary RAM/GPU process snapshotting.

## Playtime

Playtime starts after READY and is persisted per package under:

```text
%LOCALAPPDATA%\ZERO\Playtime\<package-id>.txt
```

The current V3 slice records session playtime. Accumulated lifetime playtime and richer statistics are follow-up work.

## Overlay protocol

The Runtime can send:

```text
OVERLAY 1
OVERLAY 0
```

through the authenticated session channel. The SDK exposes an overlay callback to the game, allowing games to pause, reduce audio, or release input consistently when the ZERO system overlay takes focus.

## SDK API

Current native C++ SDK client:

```cpp
zero::sdk::Client client;
client.Initialize(error);
client.ReportReady(error);
client.SetResumeActivity(context, error);
client.SetOverlayCallback(...);
client.Poll(error);
```

The SDK is intentionally game-agnostic and can later be wrapped for Unreal Engine, Unity, Godot, and other engines.

## Security boundary

Runtime V3 improves trusted communication between a launched game and ZERO platform services, but it is not a hardened VM or hostile-code sandbox.

Security properties currently include:

- process containment through Windows Job Objects
- isolated writable storage paths
- package-bound runtime session identity
- random session authentication token
- token kept off command-line arguments
- session-scoped local named pipe
- Runtime-owned authorization context

Future hardening should include explicit pipe ACLs, stricter protocol framing, message size limits on both ends, token removal after successful authentication, handshake deadlines, executable/signature verification, and fuzz testing.

## Validation game

`ZeroTestGame.exe` validates:

1. Runtime-created storage paths
2. SDK initialization
3. authenticated Runtime V3 handshake
4. READY reporting
5. Resume metadata publication
6. clean exit back to ZERO

The validation executable is not part of the platform architecture and Runtime V3 must never depend on its identity or behavior.

## Next hardening targets

- inject IPC bootstrap before process resume instead of using startup polling
- explicit named-pipe security descriptor / current-user ACL
- handshake timeout with `HandshakeTimeout` runtime result
- accumulated playtime database
- load Resume context back into newly launched game sessions
- real ZERO in-game overlay UX
- structured binary or length-prefixed protocol
- SDK ABI/version policy
- Unreal/Unity adapters
- automated IPC and failure tests
