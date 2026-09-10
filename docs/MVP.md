# ZERO Virtual Console MVP

Current coded slice:

- Native Win32/Direct2D shell
- Warm, non-sci-fi visual direction
- Home / Library / Game Detail / Settings
- XInput + keyboard navigation
- Manifest-driven game discovery
- Native EXE launch using CreateProcessW
- Single active runtime session
- Runtime process monitoring and crash/exit distinction
- Per-game ZeroSave / ZeroCache / ZeroTemp locations
- Persistent local settings
- Development library refresh
- Runtime validation test EXE

## Library location

`%LOCALAPPDATA%\\ZERO\\Library`

Each game gets its own folder with `zero.manifest.json` and its executable.

## MVP scope boundary

No Store, Cloud, Zero ID backend, social, payments, online entitlements, DRM, or custom `.zvc` runtime in this slice.

The MVP exists to prove the platform loop reliably across many games:

```text
Install ZERO
 -> Launch ZERO
 -> Discover registered game
 -> Launch native Game.exe through Runtime V2
 -> Play
 -> Save
 -> Exit or crash
 -> Return safely to ZERO
```

No single game, including WanderTales, may be hardcoded into the platform runtime.
