# ZERO Virtual Console Architecture

ZERO Virtual Console is a standalone, game-agnostic Windows platform. Individual games integrate with ZERO; they do not define the platform.

```text
Windows 11 x64
   |
ZeroVirtualConsole.exe
   |-- Home / Library / Game Detail / Settings
   |-- XInput + keyboard focus navigation
   |-- GameRegistry
   |     `-- %LOCALAPPDATA%/ZERO/Library/<Game>/zero.manifest.json
   |
   `-- RuntimeSession V2
          |-- validate package identity and executable path
          |-- isolated local storage under %LOCALAPPDATA%/ZERO
          |-- preserve Windows environment + inject ZERO session variables
          |-- CreateProcessW(CREATE_SUSPENDED)
          |-- assign process to Windows Job Object
          |-- resume game process
          |-- monitor lifecycle and exit code
          |-- contain child processes
          `-- distinguish clean exit, crash, and forced termination
```

## Platform rule

Runtime code must never depend on a specific title, package ID, executable filename, art asset, save format, or gameplay system. WanderTales is one future/flagship title among many and must integrate through the same manifest/runtime contract as any other game.

## Current trust model

The current MVP validates registration, package IDs, executable location, and executable existence. Production cryptographic signing is intentionally not represented as complete yet.

## Runtime storage

Each game receives its own namespaces:

```text
%LOCALAPPDATA%/ZERO/
  Saves/<package-id>/
  Cache/<package-id>/
  Temp/<package-id>/<session-id>/
  Runtime/Sessions/<session-id>.json
```

## Next engineering gates

1. Replace the bootstrap manifest reader with strict JSON parsing and schema validation.
2. Add authenticated local SDK IPC and a game READY handshake.
3. Add structured rotating logs.
4. Add persistent playtime and game-authored Resume metadata.
5. Add real image loading for hero and cover art.
6. Add borderless fullscreen and display-safe scaling.
7. Add atomic local import/install flow.
8. Add unit and integration tests for manifest validation, registry behavior, runtime lifecycle, settings, and failure recovery.
9. Add Windows code-signing verification when the distribution model is ready.
