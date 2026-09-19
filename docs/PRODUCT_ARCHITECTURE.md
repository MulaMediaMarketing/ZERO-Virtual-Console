# ZERO Player — Product Architecture

ZERO Player is structured as a product, not as a set of loosely connected pages.

The architecture is intentionally layered so that the user experience remains fast and cohesive while business truth, online authority, package security, and runtime lifecycle stay independently testable.

## Layer 1 — Bootstrap

Responsibilities:

- process DPI/runtime setup;
- first-boot bootstrap;
- construct `ApplicationServices`;
- construct and run `App`;
- no feature business logic.

`main.cpp` must trend toward a small bootstrap file. Window chrome implementation should live with the shell/window layer rather than accumulating in bootstrap.

## Layer 2 — Application composition

`ApplicationServices` owns long-lived production services and adapters.

It is the one place where concrete implementations are selected:

- local versus online identity;
- disconnected versus production Store transport;
- disconnected versus production social transport;
- cloud transport;
- downloads/content delivery;
- runtime;
- settings;
- package import;
- captures;
- input;
- production shell.

The application shell receives references/interfaces from this composition root. Individual pages must not instantiate providers.

## Layer 3 — Shell and experience modules

The shell owns:

- window lifecycle;
- permanent navigation;
- contextual navigation/back stack;
- focus and input routing;
- page transitions;
- global top bar;
- overlay/modals;
- global notifications;
- composition of feature view state.

Each feature exposes state and commands instead of embedding domain rules in drawing code.

Target feature module pattern:

```text
features/<feature>/
  <Feature>State.h
  <Feature>Controller.h
  <Feature>Controller.cpp
  <Feature>ViewModel.h
  <Feature>Acceptance.cpp
```

Renderer code consumes the view model and emits commands only.

## Layer 4 — V5 domains and platform services

Domain code owns rules and validation. Examples:

- content identity and entitlement;
- package lifecycle;
- library/install state;
- download/update state machine;
- achievements/profile authority;
- commerce quote validation;
- social/presence/party authority;
- cloud save conflict resolution;
- cloud gaming allocation validation;
- notification authority;
- update authority;
- device authority.

Domain code must not depend on HWND, Direct2D, page coordinates, or view state.

## Layer 5 — Infrastructure adapters

Infrastructure translates external systems into domain contracts:

- HTTP/service clients;
- ZERO ID transport;
- catalog/commerce transport;
- social transport;
- CDN/content delivery;
- cloud services;
- filesystem;
- SQLite;
- Windows process APIs;
- XInput;
- cryptography/signing/trust.

Adapters may fail. Domain state must remain valid when they do.

## Layer 6 — Runtime Core

Runtime remains isolated from launcher/store UI concerns.

It owns:

- launch policy;
- capability grant;
- process lifecycle;
- secure IPC;
- READY/heartbeat state;
- Resume;
- crash/hang supervision;
- playtime/session finalization;
- runtime achievements bridge;
- foreground recovery.

The shell asks Runtime to launch/terminate; it does not reproduce runtime state machines itself.

## Data-flow rule

Normal product data flows one way:

```text
Infrastructure -> Domain/Authority -> Feature Controller/ViewModel -> Renderer
Renderer/Input -> Command -> Feature Controller/Authority
```

The renderer never writes authoritative business state directly.

## Online authority rule

The Windows client may cache and project server state, but it may not mint online/commercial truth.

Server-authoritative concerns include:

- ZERO ID sessions;
- catalog publication;
- pricing and checkout quotes;
- managed entitlements;
- social graph/presence/invites/parties;
- cloud saves and cloud sessions;
- remote devices;
- global achievements/score where configured;
- publisher trust/revocation where managed online.

When a service is disconnected, the associated experience remains visually complete but functionally fail-closed.

## UX architecture rule

ZERO Player should feel immediate like a game player while still supporting deep library and store management.

That requires:

- fast startup with local state available immediately;
- no blocking network dependency for the shell;
- cached projections clearly distinguished from authoritative availability;
- background refresh through service controllers;
- deterministic focus/navigation;
- artwork-first Home/Discover/Library surfaces;
- actions driven by authoritative capability state;
- consistent loading, empty, disconnected, error and recovery states.

## Refactor target for existing files

### `main.cpp`

Target: bootstrap only.

Move out:

- custom header/window implementation;
- sidebar icon painting;
- preview renderer;
- account popup behavior.

### `App.cpp`

Target: shell/application coordinator.

Move out:

- feature-specific input branching;
- Store/Friends/Capture/Settings business decisions;
- launch-detail presentation decisions that belong to feature controllers.

### `AppPages.cpp`

Target: composition/render dispatch only.

Move each visible destination into a feature renderer/controller pair. Drawing code should not query multiple authorities and decide product policy inline.

### Provider ownership

Move all concrete provider creation to `ApplicationServices`. App and features consume interfaces/references.

## Testing strategy

Three levels are mandatory:

1. **Domain acceptance** — rules and authority validation without UI.
2. **Feature acceptance** — commands -> state/view model behavior.
3. **Product acceptance** — real executable, input parity, visual shell, launch/recovery, install/update, physical qualification.

CI passing level 1 or 2 does not substitute for physical level 3 qualification.

## Dependency direction

Allowed:

```text
Bootstrap -> Composition -> Shell/Features -> Domains <- Infrastructure
                              |
                              -> Runtime interfaces
```

Forbidden:

- domain -> renderer;
- service client -> page coordinates;
- feature -> another feature's private state;
- UI -> direct database writes;
- UI -> minting online authority;
- parallel legacy and V5 authorities for the same concern.

## Completion target

The refactor is complete when:

- `ApplicationServices` is the only production composition root;
- `main.cpp` is small bootstrap code;
- App no longer constructs concrete feature providers;
- feature-specific business behavior is outside rendering files;
- all permanent pages have feature state/controller boundaries;
- the project documentation consistently describes ZERO Core V5 and 12 permanent destinations;
- exact-head Windows Build and CodeQL remain green throughout the migration;
- the signed, physically qualified artifact passes final E2E acceptance.
