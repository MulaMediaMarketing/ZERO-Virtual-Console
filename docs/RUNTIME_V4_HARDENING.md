# ZERO Runtime V4 Hardening

This hardening slice extends Runtime V4 without changing ZERO's game-agnostic platform model.

## Added in this slice

- Structured crash-report persistence under `%LOCALAPPDATA%\\ZERO\\CrashReports\\<package-id>\\<session-id>.json`
- Crash records include package/session identity, game version, executable, PID, exit code, playtime, and forced-termination state
- Authenticated SDK achievement command routing
- Runtime-level validation for achievement IDs and titles
- Platform-owned local achievement persistence through `AchievementStore`
- `ZeroSdk::Client::UnlockAchievement(...)`
- Updated `ZeroTestGame` validation path

## Security boundary

Games do not write directly to ZERO achievement storage. A game sends an authenticated session-scoped IPC command, Runtime V3 forwards the event, and Runtime V4 decides whether to persist it.

The runtime session token remains outside the process command line.

## Validation

`ZeroTestGame` now validates:

1. SDK authentication
2. READY handshake
3. Resume metadata publication
4. Achievement unlock routing
5. clean return to ZERO

## Remaining V4 hardening backlog

These are still intentionally separate work items and should not be represented as complete:

- length-prefixed/framed IPC replacing the current line protocol
- reconnectable SDK transport
- heartbeat/liveness timeout policy
- Resume launch context delivered back into the game
- minidump generation and symbol-aware crash processing
- production system-overlay UI and controller shortcut
- achievements manifest/catalog validation and progress achievements
- SQLite migration from bootstrap JSON/text persistence

ZERO Runtime remains platform-generic; no individual game is hardcoded into these services.
