# ZERO Virtual Console

Native Windows foundation for **ZERO Virtual Console**, a standalone software-defined game console platform for PC.

> The PC is the hardware. ZERO is the console layer.

ZERO is game-agnostic. It is designed to host many current and future games; no individual title owns or defines the platform architecture.

## Runtime V2 architecture

`Windows -> ZERO Shell -> Game Manifest -> ZERO Runtime V2 -> native Game.exe`

Games are discovered through `zero.manifest.json` files placed in:

`%LOCALAPPDATA%\\ZERO\\Library\\<GameName>\\`

Runtime V2 currently provides:

- manifest-driven game discovery
- native Windows `.exe` launching
- unique runtime sessions
- Windows Job Object process containment
- suspended creation -> containment -> resume launch sequence
- controlled graceful/forced shutdown
- crash-vs-clean-exit detection
- per-game save/cache/temp isolation
- persistent runtime session diagnostics
- controller/keyboard shell navigation
- game-agnostic runtime contracts

## Build with VS Code on Windows

Requirements:

- Windows 11 x64
- VS Code
- CMake Tools extension
- Microsoft C/C++ extension
- Visual Studio Build Tools 2022 with **Desktop development with C++**
- Windows 11 SDK
- CMake 3.24+

From the VS Code terminal:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Build outputs include `ZeroVirtualConsole.exe` and `ZeroTestGame.exe`.

## Register the validation game

1. Create `%LOCALAPPDATA%\\ZERO\\Library\\ZeroRuntimeTest`.
2. Copy `ZeroTestGame.exe` into that folder.
3. Copy `sample_game\\zero.manifest.json` beside it.
4. Launch `ZeroVirtualConsole.exe`.
5. Press F5 to refresh if ZERO is already open.

The validation game is only a test harness. Runtime code must never depend on its package ID, title, executable name, content, or behavior.

## Controls

- Left/Right: top navigation
- Up/Down: Library selection
- A / Enter: select / launch
- B / Escape: back
- F5: refresh Library
- Ctrl+Q: exit ZERO

## MVP boundary

This repository is the coded platform foundation, not a claim that the entire commercial ecosystem is complete. Store, payments, production Zero ID, Cloud, Link/social, public SDK distribution, DRM, and other post-MVP services remain intentionally outside the current runtime slice.
