# ZERO Virtual Console

Native Windows foundation for **ZERO Virtual Console**, a standalone, game-agnostic software-defined game console platform for PC.

> The PC is the hardware. ZERO is the console layer.

ZERO is designed to host many current and future games. No individual title owns, defines, or is hardcoded into the platform architecture.

## Runtime V4.1 architecture

`Windows 11 x64 -> ZERO Shell -> Game Manifest -> Runtime V4.1 -> native Game.exe`

Runtime V4.1 provides:

- manifest-driven game discovery and strict package validation;
- native Windows `.exe` launch through suspended creation and Windows Job Object containment;
- authenticated local Runtime/SDK session plumbing and READY lifecycle;
- per-game Saves/Cache/Temp isolation;
- canonical local game/session statistics, Resume, achievements, and diagnostics;
- SHA-256 package integrity verification before launch;
- local trusted-publisher verification using ECDSA P-256/SHA-256 through Windows CNG;
- safe package repair/re-import and transactional installer/update rollback;
- four-slot XInput arbitration plus keyboard fallback;
- production shell navigation, overlay lifecycle, launch recovery, First Boot, Settings, Friends, Captures, Store provider boundaries, and accessibility state;
- automated M1 acceptance, architecture-policy measurement, deterministic release packaging, and physical RC qualification tooling.

Games are discovered under:

`%LOCALAPPDATA%\\ZERO\\Library\\<GameName>\\zero.manifest.json`

Player/runtime data remains under `%LOCALAPPDATA%\\ZERO` and is separated from replaceable program files.

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

The architecture report measures source size, acceptance coverage, binary hashes, documentation alignment, and policy violations. CI may produce an automated PASS, but **CI can never qualify the RC**. Final qualification requires the documented real Windows 11 x64 + physical XInput controller journey for the exact candidate commit.

See:

- `docs/ARCHITECTURE.md`
- `docs/PRODUCTION_ARCHITECTURE_STANDARD.md`
- `docs/RUNTIME_V4_1_RC_GATE.md`
- `docs/PACKAGE_SECURITY.md`
- `docs/PACKAGE_TRUST.md`

## Permanent shell destinations

The production shell contract exposes exactly six permanent top-level destinations:

**Home · Library · Store · Friends · Captures · Settings**

Game Detail, Import, and Achievements are secondary pages and do not redefine top-level navigation.

## Platform boundary

The Runtime V4.1/local-console foundation does not falsely claim completion of the entire online commercial ecosystem. Production Store commerce, cloud Zero ID, Zero Link backend, DRM, anti-cheat, native video capture/encoding, public publisher PKI/portal, public SDK distribution, ARM64, HDR certification, and non-Windows targets remain separate milestones until implemented.
