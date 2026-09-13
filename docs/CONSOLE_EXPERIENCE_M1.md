# ZERO Console Experience Milestone 1

This milestone turns the existing runtime foundation into a usable console experience without changing ZERO's game-agnostic architecture.

## Slice 1: Package Contract + Import

Implemented in this branch:

- versioned manifest model (`schema: 1`)
- minimum runtime major declaration
- package ID validation
- executable containment validation
- artwork path containment validation
- staged folder import
- duplicate package protection
- `.staging` isolation from the visible Library
- library scan rejects invalid packages

## Current manifest contract

```json
{
  "schema": 1,
  "minimum_runtime_major": 4,
  "package_id": "zero.publisher.game",
  "title": "Example Game",
  "version": "1.0.0",
  "executable": "Game.exe",
  "hero_image": "Assets/hero.jpg",
  "icon_image": "Assets/icon.png",
  "logo_image": "Assets/logo.png",
  "zero_resume": true,
  "zero_achievements": true,
  "zero_overlay": true,
  "zero_input": true
}
```

## Milestone 1 acceptance path

1. Install and launch ZERO.
2. Navigate using controller.
3. Import a compatible game folder.
4. ZERO stages and validates the package.
5. Valid package appears in Library.
6. Launch creates a Runtime V4 session.
7. SDK authenticates and reports READY.
8. Game can save, unlock achievements, and publish Resume metadata.
9. Exit returns to ZERO with updated platform state.
10. Crash leaves ZERO alive with diagnostics.

## Next slices

- shell Import UI wired to `GameImportService`
- real artwork loading with WIC
- Resume row and Resume launch context
- borderless fullscreen console mode
- first-boot flow
- controller-opened native ZERO overlay
- installer/uninstaller productization

The package parser remains deliberately dependency-free in this slice. A fully standards-compliant JSON parser/schema layer should replace the bootstrap field scanner before public SDK certification.
