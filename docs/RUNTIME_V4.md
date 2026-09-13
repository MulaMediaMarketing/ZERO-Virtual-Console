# ZERO Runtime V4

Runtime V4 is the first ZERO platform-services coordinator built on top of the stable Runtime V3 process and IPC foundation.

## Goals

Runtime V4 remains fully game-agnostic. Every title is identified only through its package manifest and runtime session identity.

## Added in V4

- persistent per-game platform state
- cumulative playtime tracking
- launch count tracking
- last-played timestamp
- last session identity
- last exit code and crash-state persistence
- local achievement persistence foundation
- Runtime V4 coordinator layered over V3

## Architecture

```text
ZERO Shell
  -> Runtime V4
     -> PlatformStateStore
     -> AchievementStore
     -> Runtime V3
        -> authenticated SDK IPC
        -> ResumeStore
        -> Runtime V2 process host
           -> Windows Job Object
           -> Game.exe
```

## Storage

Platform state is stored under:

`%LOCALAPPDATA%\\ZERO\\PlatformState\\<package-id>.json`

Achievements are stored under:

`%LOCALAPPDATA%\\ZERO\\Achievements\\<package-id>.jsonl`

These are local MVP services. A future ZERO account/cloud layer may synchronize them, but Runtime V4 does not require network services.

## Next V4 gates

The remaining V4 hardening work is:

1. framed binary-safe IPC messages instead of newline-delimited commands
2. heartbeat timeout and reconnect negotiation
3. SDK achievement unlock command path wired into AchievementStore
4. Resume launch context delivered back to games at startup
5. structured minidump/crash diagnostic capture
6. real in-game ZERO overlay UI surfaced by the shell
7. schema migration/versioning for persisted platform state

Runtime V4 should not be described as a hardened hostile-code sandbox or complete commercial platform backend.
