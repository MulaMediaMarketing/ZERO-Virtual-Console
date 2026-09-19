# ZERO Player UI Visual Test Fixtures

Everything under `assets/placeholders/` is fictional development-only content for visual regression and layout stress testing.

It must never be loaded by the normal production Store, Discover, Library, entitlement, commerce, social, cloud, or identity paths. Production continues to fail closed when authoritative service data is unavailable.

## Fixture goals

- exercise long and short game titles;
- exercise long developer/genre/description fields;
- validate 1280x720 through 4K layouts and 100-200% DPI;
- verify single-line header controls never wrap or collide;
- verify account names use bounded layouts and ellipsis;
- verify download/error strings cannot overlap neighboring controls.

The three fictional entries are Neon District, Iron Horizon: Black Meridian, and Wild Signal. Every metadata document carries `uiTestOnly: true`.
