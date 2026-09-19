# ZERO Player — Locked Production User Interface

Status: **LOCKED**

This document is the authoritative visual and interaction contract for the ZERO Player production UI. The production renderer, web previews, acceptance references, and future page integrations must conform to this direction. This is not a temporary mockup theme and must not be replaced by generic launcher, programmer, light-dashboard, or unrelated Apple-style UI without an explicit product decision.

## Product feeling

ZERO Player must feel like a **next-generation gaming console environment inside a PC**, not another desktop launcher.

The visual identity is:

- dark;
- cinematic;
- premium;
- sleek;
- minimal but information-rich;
- gaming-first;
- immersive;
- high-end entertainment software;
- unmistakably ZERO.

The UI must preserve the strong ZERO brand and make the product feel like its own gaming environment.

## Locked visual language

### Base palette

Primary surfaces:

- deep black;
- near-black navy;
- graphite;
- dark charcoal;
- metallic gray.

Accent treatment:

- electric blue;
- cyan;
- subtle cool violet only where appropriate;
- controlled white glow.

Accent lighting must be used selectively. Do not turn the application into RGB rainbow UI.

### ZERO branding

- `ZERO PLAYER` is the primary application identity.
- The ZERO wordmark must be prominent and premium.
- The `O` may use the illuminated ZERO ring treatment.
- Branding must remain clean, recognizable, and high-contrast.

## Locked application composition

The primary desktop experience is designed around **1920×1080 widescreen** and scales responsibly above and below that baseline.

The shell uses three persistent zones:

1. **Left navigation sidebar**
2. **Main cinematic content area**
3. **Thin top account/system bar**

A light theme with navigation spread across the top is not the ZERO Player production direction.

## Left navigation sidebar

The left sidebar is a permanent ZERO shell element.

It must:

- remain visually dark and separated from the main content;
- use icon + label navigation;
- provide a strong blue/cyan focus or selection treatment;
- support controller, keyboard, and mouse focus;
- be capable of collapsing to an icon-focused form without changing navigation authority;
- keep the ZERO PLAYER brand at the top;
- preserve clear grouping between gaming destinations and identity/system destinations.

The permanent V5 destinations remain authoritative and must be represented by the final sidebar/navigation system:

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

The exact visual grouping may place Profile / Devices / Settings in the lower system section, but this must not create a second navigation authority or remove any permanent V5 destination.

## Top account / system bar

The top bar is thin and unobtrusive. It belongs above the main content, not in place of the left navigation.

It must support the production equivalents of:

- global search;
- notifications;
- download activity;
- connected-device status;
- user avatar;
- username / ZERO ID presentation;
- online / local / disconnected state;
- appropriate window controls on Windows.

System truth must be real. Disconnected services must never display fabricated online state.

## Home screen

The Home screen is cinematic and personalized.

### Featured hero

A large featured-game artwork area dominates the upper content region.

The hero supports:

- ZERO ORIGINAL / source labeling when authoritative;
- large game title treatment;
- short description;
- `PLAY`;
- `PLAY IN CLOUD` only when cloud launch is genuinely available;
- `MORE INFO`;
- `INSTALL` when the title is owned but not installed;
- package / cloud / controller / save status where those states are authoritative.

Artwork should occupy substantial visual space. The Home page must not degrade into rows of plain text or generic settings-style cards.

### Continue Playing

Below the hero, use large horizontal game cards showing the production equivalents of:

- game artwork;
- title;
- last played;
- playtime where useful;
- cloud-save state when authoritative;
- immediate Play / Resume affordance.

### Recently Played / ZERO Originals

Use artwork-led horizontal carousels or rows. Cards should feel console-grade, with minimal text and strong imagery.

## Card and panel language

Cards:

- dark translucent or deep solid surfaces;
- rounded corners;
- restrained cool-blue edge/focus lighting;
- cinematic artwork;
- minimal text;
- clear controller focus state;
- subtle hover/focus enlargement where motion settings allow.

Panels:

- dark layered surfaces;
- slight blur only where useful;
- clean thin borders;
- strong hierarchy;
- no excessive glassmorphism.

Primary actions use bright but controlled blue/cyan emphasis. Secondary actions use dark translucent surfaces with clear borders/focus.

## Typography

Use a modern geometric sans-serif direction.

- Headlines: bold, wide, premium.
- Navigation: clean uppercase or title case.
- Secondary data: thinner and understated.
- Game titles may use custom art/logo treatment when supplied by the package/catalog.
- ZERO must always feel strong and iconic.

## Controller-first interaction

ZERO Player must work as a couch-readable console UI even though it is a Windows application.

Every production page must support:

- mouse;
- keyboard;
- controller;
- future ZERO hardware input through the same logical action model.

Requirements:

- visible focus at all times during controller/keyboard navigation;
- large hit/focus targets;
- predictable directional navigation;
- no mouse-only interactions for primary actions;
- no hidden action that requires tiny desktop UI precision.

## Micro-interactions

Motion is premium and restrained:

- selected cards may enlarge slightly;
- blue/cyan edge glow may intensify on focus;
- hero artwork may shift subtly;
- download indicators may animate;
- cloud connection may pulse softly;
- Play may use a restrained light response.

Reduced-motion settings must disable or simplify nonessential movement.

## Contextual pages

The same visual system applies to:

- Game Detail;
- Wishlist;
- Checkout;
- Notifications;
- Import;
- capture viewer/editor surfaces;
- launch/recovery experiences;
- overlays and modals.

Contextual pages may change content density, but they may not introduce a different product skin or a second navigation model.

## Capture experience

Capture is a first-class ZERO destination, not a utility afterthought.

It follows the locked dark cinematic shell and may contain:

- media tabs;
- recent capture grid;
- selected capture preview;
- metadata;
- clip timeline where supported;
- Record;
- Screenshot;
- Trim;
- Share;
- Export;
- More tools.

Only actions that are actually implemented should be enabled. Unsupported cloud/share/edit actions must remain truthful and unavailable rather than simulated.

## Profile

Profile should feel like a gaming identity card and can include authoritative versions of:

- ZERO ID;
- avatar;
- banner;
- level / progression;
- total playtime;
- achievements;
- games owned;
- favorites;
- friends;
- recent activity.

Do not fabricate statistics while backend identity/social services are disconnected.

## Devices

Devices uses the same premium dark shell and is reserved for authoritative device information such as:

- ZERO Player PC;
- ZERO Stick;
- future ZERO hardware;
- connection status;
- version / firmware;
- rename/disconnect/security actions where supported.

No fake connected hardware is permitted.

## Settings

Settings remains visually part of ZERO Player.

It must not look like a separate programmer/admin utility.

Expected categories include the production equivalents of:

- Account;
- Downloads;
- Cloud Gaming;
- Controller;
- Video;
- Audio;
- Notifications;
- Accessibility;
- Devices;
- Privacy;
- About ZERO.

Only categories backed by real functionality should expose active controls; disconnected capabilities should present polished unavailable states.

## Startup and authentication direction

Startup:

- dark ZERO screen;
- ZERO / PLAYER branding;
- illuminated ring animation where motion is enabled;
- transition into identity/welcome or Home.

Authentication when real Zero ID is available:

- minimal dark cinematic presentation;
- Sign In;
- Create Account;
- QR flow where supported;
- no fake logged-in online account when only local identity exists.

## Truthful disconnected states

The locked UI does not require fake backend content.

If a service is unavailable, the page still receives the full premium ZERO visual treatment but displays a deliberate, understandable disconnected/unavailable state.

Never populate production pages with fake:

- Store products;
- recommendations;
- friends;
- online presence;
- cloud saves;
- cloud gaming sessions;
- download jobs;
- devices;
- account statistics;
- entitlements.

## Explicitly rejected directions

The following are not acceptable for ZERO Player without an explicit product redesign decision:

- bright/light dashboard as the primary product theme;
- top-navigation-only layout;
- generic SaaS dashboard styling;
- programmer/debug UI;
- plain text-heavy launcher pages;
- excessive RGB neon;
- childish mobile-game visual language;
- unrelated Apple-style productivity UI;
- Steam/Xbox/PlayStation/Epic clones;
- a different visual system per page;
- placeholder/fake content presented as real service data.

## Production acceptance rule

A page is not considered visually integrated merely because its controller exists or because navigation reaches it.

A visible page integration is complete only when:

1. it uses this locked ZERO Player shell and visual language;
2. it is controller/keyboard/mouse navigable;
3. it has correct focus and Back behavior;
4. all enabled actions are real;
5. unavailable services fail closed with polished ZERO UI;
6. it does not add a second navigation/state authority;
7. it passes the relevant UX/acceptance gate.

## Change control

This UI direction is locked for the production-closure phase.

Engineering work may improve implementation quality, responsiveness, accessibility, performance, and fidelity, but must not silently redesign the product.

Any material change to the locked composition, visual language, navigation placement, or overall identity requires an explicit product-level decision and an update to this document.
