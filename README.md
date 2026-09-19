# ZERO Player

Native Windows foundation for **ZERO Player**, a standalone, game-agnostic software-defined gaming platform for PC.

> The PC is the hardware. ZERO is the console layer.

ZERO is designed to host many current and future games. No individual title owns, defines, or is hardcoded into the platform architecture.

## ZERO Core V5

The production architecture is **ZERO Core V5**.

At a high level:

`Windows 11 x64 -> ZERO Player Shell -> V5 Platform Authorities -> Package/Runtime Core -> Game`

ZERO Core V5 provides or coordinates:

- authoritative ShellKernel navigation and contextual Back history;
- manifest-driven game discovery and strict package validation;
- a single package lifecycle for import/install/update/repair;
- native Windows game launch with process/session supervision;
- authenticated Runtime/SDK IPC and READY lifecycle;
- isolated per-game Saves/Cache/Temp data;
- canonical local session statistics, Resume, achievements, and diagnostics;
- SHA-256 package integrity verification before launch;
- trusted-publisher verification using ECDSA P-256/SHA-256 through Windows CNG;
- crash/hang supervision and recovery;
- XInput controller support plus keyboard/mouse parity;
- local-first Home, Discover, Library, Downloads, Capture, Profile, Devices and Settings experiences;
- explicit fail-closed boundaries for Store, ZERO ID, ZERO Link/social, cloud and other server-authoritative systems;
- automated acceptance, architecture-policy measurement, deterministic release packaging, SBOM generation, and physical RC qualification tooling.

Games are discovered under:

`%LOCALAPPDATA%\\ZERO\\Library\\<GameName>\\zero.manifest.json`

Player/runtime data remains under `%LOCALAPPDATA%\\ZERO` and is separated from replaceable program files.

## Engineering entrypoint

New engineers should start with:

- `docs/ENGINEERING_START_HERE.md`
- `docs/PRODUCT_ARCHITECTURE.md`
- `docs/ZERO_CORE_ARCHITECTURE_V5.md`
- `docs/ZERO_PLAYER_UI_LOCK.md`
- `docs/PRODUCTION_CLOSURE_POLICY.md`

The codebase follows one-authority-per-concern. UI/rendering code is presentation; business truth belongs in domain/service authorities. Long-lived production services are centralized through the application composition root rather than instantiated independently by pages.

## Build

Requirements:

- Windows 11 x64
- Visual Studio Build Tools 2022 with Desktop development with C++
- Windows 11 SDK
- CMake 3.24+

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```

## Production verification

A candidate is not accepted because it compiles. Production verification is evidence-driven:

```powershell
./tools/verify-production-architecture.ps1 -BuildRoot ./build -ReportDir ./build/ArchitectureReports
./tools/verify-m1-automated.ps1 -BuildRoot ./build -ReportDir ./build/AcceptanceReports
./tools/run-rc-qualification.ps1 -BuildRoot ./build -EvidenceDir ./build/QualificationEvidence -NonInteractiveValidation
./tools/package-release.ps1 -BuildRoot ./build -OutputDir ./build/ReleaseBundle
```

CI may produce an automated PASS, but CI can never qualify a production RC. Final qualification requires the documented real Windows 11 x64 + physical XInput controller journey for the exact candidate commit, followed by production signing and signed-artifact validation.

## Permanent ZERO Player destinations

The production shell exposes exactly twelve permanent top-level destinations:

1. Home
2. Discover
3. Store
4. Library
5. Cloud Play
6. Downloads
7. Friends
8. Achievements
9. Capture
10. Profile
11. Devices
12. Settings

Game Detail, Import, Wishlist, Checkout and Notifications are contextual/supporting destinations and do not redefine top-level navigation authority.

## Platform boundary

ZERO Player never fabricates online or commercial authority.

Production Store commerce, online ZERO ID, ZERO Link/social, ZERO Cloud, remote content/CDN delivery, remote-device services and other network-backed features remain disconnected until their real production providers, credentials and infrastructure are deployed and qualified.

Disconnected services must fail closed while the shell remains fully usable for supported local-first functionality.

## Release rule

ZERO Player is releasable only when one exact commit satisfies all of the following:

- automated acceptance is green;
- Windows Build is green;
- CodeQL is green;
- required review/governance gates pass;
- physical Windows qualification passes for the exact SHA;
- the production executable is Authenticode signed;
- the exact signed artifact passes final E2E acceptance.
