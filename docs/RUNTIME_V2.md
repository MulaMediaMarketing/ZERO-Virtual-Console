# ZERO Runtime V2

ZERO Runtime V2 is the game-agnostic native Windows execution layer for ZERO Virtual Console.

## V2 guarantees

- Native Windows x64 `.exe` payloads remain the execution format.
- Runtime is not tied to WanderTales or any other game.
- One active game session at a time for the MVP.
- Each launch receives a unique session ID.
- Each game receives isolated save/cache/temp roots under `%LOCALAPPDATA%\\ZERO`.
- The normal Windows environment is preserved and ZERO-specific variables are added.
- The primary game process is created suspended, placed into a Windows Job Object, then resumed.
- Closing the Job Object kills remaining child processes, preventing orphaned game processes.
- ZERO first requests graceful shutdown through top-level `WM_CLOSE`, then force-terminates the job after a timeout.
- Runtime distinguishes clean exit, crash, failed launch, and forced termination.
- Runtime session records are atomically written to `%LOCALAPPDATA%\\ZERO\\Runtime\\Sessions` for diagnostics and future Resume/history services.
- Executables must remain inside their registered game root and must currently use `.exe`.
- Package IDs are validated before being used as filesystem namespace keys.

## Runtime environment contract

Runtime V2 injects these variables into each game process while preserving the inherited Windows environment:

- `ZERO_RUNTIME=2`
- `ZERO_SESSION_ID`
- `ZERO_PACKAGE_ID`
- `ZERO_CONTENT_ROOT`
- `ZERO_SAVE_ROOT`
- `ZERO_CACHE_ROOT`
- `ZERO_TEMP_ROOT`

These values are convenience/bootstrap information, not security credentials.

## Launch lifecycle

```text
Game selected
 -> validate manifest identity and executable
 -> create isolated storage
 -> create session record
 -> create Windows containment Job Object
 -> CreateProcessW(CREATE_SUSPENDED)
 -> assign process to Job Object
 -> ResumeThread
 -> mark session Running
 -> poll process health
 -> clean exit / crash / forced termination
 -> persist final session record
 -> return control to ZERO Shell
```

## Still deferred

Runtime V2 does not yet implement authenticated SDK IPC, game readiness handshakes, production code signing, cloud saves, Store entitlements, anti-cheat, or a hardened Windows sandbox. Those remain post-MVP or later Runtime milestones.
